import json
import subprocess
import tempfile
import time
import random
import os
from io import BytesIO

import cbor2 as cbor
import usb.core

from viaems.vcd import dump_vcd
from viaems.validation import enrich_log

class ViaemsInterface:
    def recv_until_id(self, id, max_messages=1000):
        while True:
            result = self.recv()
            if "id" in result and int(result["id"]) == id:
                return result
            max_messages -= 1
            if max_messages == 0:
                return None

    def structure(self):
        id = random.randint(0, 1024)
        self.send(
            {
                "id": id,
                "type": "request",
                "method": "structure",
            }
        )
        result = self.recv_until_id(id)
        return result

    def get(self, path):
        id = random.randint(0, 1024)
        self.send(
            {
                "id": id,
                "type": "request",
                "method": "get",
                "path": path,
            }
        )
        result = self.recv_until_id(id)
        return result

    def set(self, path, value):
        id = random.randint(0, 1024)
        self.send(
            {
                "id": id,
                "type": "request",
                "method": "set",
                "path": path,
                "value": value,
            }
        )
        result = self.recv_until_id(id)
        return result
    


class SimConnector(ViaemsInterface):
    def __init__(self, binary):
        self.binary = binary
        self.process = None

    def start(self, replay=None):
        args = [self.binary]
        if replay:
            args.append("-i")
            args.append(replay)

        self.process = subprocess.Popen(
            args,
            bufsize=-1,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def kill(self):
        if not self.process:
            return
        self.process.kill()
        self.process.communicate()
        self.process.wait()

    def send(self, payload):
        binary = cbor.dumps(payload)
        self.process.stdin.write(binary)
        self.process.stdin.flush()

    def recv(self):
        result = cbor.load(self.process.stdout)
        return result

    def execute_scenario(self, scenario):
        tf = open(f"scenario_{scenario.name}.inputs", "w")
        for ev in scenario.events:
            tf.write(ev.render())
        tf.close()

        self.start(replay=tf.name)
        results = []
        while True:
            try:
                msg = self.recv()
                results.append(msg)
            except EOFError:
                break

        for line in self.process.stderr:
            print(line)

        results.sort(key=lambda x: x["time"])
        dump_vcd(results, f"scenario_{scenario.name}.vcd")
        enriched_log = enrich_log(scenario.events, results)
        return enriched_log

class HilConnector(ViaemsInterface):
    def __init__(self):
        # Issue device reset
        subprocess.run(
                [
                    "viaems-fpga-tb",
                    "--cmd-outputs", 
                    "00"
                ])
        time.sleep(0.1)
        subprocess.run(
                [
                    "viaems-fpga-tb",
                    "--cmd-outputs", 
                    "80"
                ])
        time.sleep(1)


        # find the ECU device via USB
        viaems_device = usb.core.find(idVendor=0x1209, idProduct=0x2041)
        if not viaems_device:
            raise ValueError("No Viaems USB device found")
        
        if viaems_device.is_kernel_driver_active(0):
            viaems_device.detach_kernel_driver(0)

        self.device = viaems_device
        self.read_buffer = b''



    def kill(self):
        self.device.reset()

    def send(self, payload):
        binary = cbor.dumps(payload)
        self.device.write(0x1, binary)

    def recv(self):
        while True:
            try: 
                bio = BytesIO(self.read_buffer)
                result = cbor.load(bio)
                pos = bio.tell()
                self.read_buffer = self.read_buffer[pos:]
                if isinstance(result, dict):
                    return result
                continue
            except Exception as e:
                if len(self.read_buffer) > 20000:
                    self.read_buffer = self.read_buffer[1:]
                else:
                    self.read_buffer += self.device.read(0x81, 4096)

    def reconcile_logs(self, viaems_log, tb_outputs):
        # First find the timing offset between the ems logs and the test bench
        # by comparing the first trigger rising edge
        first_tb_trigger0 = None
        first_viaems_trigger0 = None

        for tbo in tb_outputs:
            time = int(tbo.rstrip().split()[2])
            value = int(tbo.rstrip().split()[3], 16)
            if value & (1 << 22):
                first_tb_trigger0 = time
                break
        if first_tb_trigger0 is None:
            raise ValueError("TB contains no trigger!")

        for ev in viaems_log:
            if ev["type"] == "event" and \
               ev["event"]["type"] == "trigger" and \
               ev["event"]["pin"] == 0:
                first_viaems_trigger0 = ev["time"]
                break
        if first_viaems_trigger0 is None:
            raise ValueError("TB contains no trigger!")

        print(first_tb_trigger0)
        print(first_viaems_trigger0)

        changes = []
        
        prev_value = 0
        for tbo in tb_outputs:
            time = int(tbo.rstrip().split()[2])
            value = int(tbo.rstrip().split()[3], 16)
            if value & 0xffff != prev_value:
                converted_time = (time - first_tb_trigger0) / (60 / 4.0) + first_viaems_trigger0
                prev_value = value & 0xffff
                changes.append({
                    "type": "event", 
                    "time": int(converted_time),
                    "event": {"type": "output", "outputs": prev_value}
                    })
        viaems_log += changes
        return viaems_log

    def execute_scenario(self, scenario):
        self.set(["test", "event-logging"], True)
        tf = open(f"scenario_{scenario.name}.inputs", "w")
        for ev in scenario.events:
            tf.write(ev.render())
        tf.close()

        # Start test bench
        tb_process = subprocess.Popen(
                [
                    "viaems-fpga-tb",
                    "--scenario", f"scenario_{scenario.name}.inputs",
                    "--output", f"scenario_{scenario.name}.outputs",
                    ], 
                )

        results = []
        while tb_process.poll() is None:
            try:
                msg = self.recv()
                results.append(msg)
            except EOFError:
                break

        tb_process.communicate()
        self.kill()

        tb_trace = open(f"scenario_{scenario.name}.outputs").readlines()

        combined = self.reconcile_logs(results, tb_trace)
        combined.sort(key=lambda x: x["time"])
        open(f"scenario_{scenario.name}.trace", "w").write("\n".join([str(e) for e
                                                                      in combined]))
        dump_vcd(combined, f"scenario_{scenario.name}.vcd")

        enriched_log = enrich_log(scenario.events, combined)
        return enriched_log




