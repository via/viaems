import cbor2 as cbor
import json
import subprocess
import tempfile
import random
import os
from io import BytesIO

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

import usb.core
class HilConnector(ViaemsInterface):
    def __init__(self):
        # find the ECU device via USB
        viaems_device = usb.core.find(idVendor=0x1209, idProduct=0x2041)
        if not viaems_device:
            raise ValueError("No Viaems USB device found")
        
        if viaems_device.is_kernel_driver_active(0):
            viaems_device.detach_kernel_driver(0)

        self.device = viaems_device
        self.read_buffer = b''

    def start(self, replay=None):
        pass

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
                    self.read_buffer += self.device.read(0x81, 512)


    def execute_scenario(self, scenario):
        pass
