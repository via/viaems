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
from viaems.events import *

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
            pipesize=1 * 1024 * 1024,
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

    def _render_target_inputs(self, scenario, file):
        render_time = 0
        for event in scenario.events:
            delay = event.time - render_time;
            match event:
                case SimToothEvent(trigger=trigger):
                    file.write(f"t {delay} {trigger}\n")
                    render_time = event.time
                case SimADCEvent(values=values):
                    adcvals = " ".join([str(v) for v in values])
                    file.write(f"a {delay} {adcvals}\n")
                    render_time = event.time
                case SimEndEvent():
                    file.write(f"e {delay}\n")

    

    def execute_scenario(self, scenario, settings=[]):
        tf = open(f"scenario_{scenario.name}.inputs", "w")
        self._render_target_inputs(scenario, tf)
        tf.close()

        self.start(replay=tf.name)
        for path, value in settings:
            self.set(path, value)
        target_msgs = []
        while True:
            try:
                msg = self.recv()
                target_msgs.append(msg)
            except EOFError:
                break

        for line in self.process.stderr:
            print(line)

        with open(f"scenario_{scenario.name}.trace", "w") as trace:
            for line in target_msgs:
                trace.write(str(line) + "\n")

        target_events = log_from_target_messages(target_msgs)
        target_events_aligned = align_triggers_to_sim(scenario.events, target_events)
        combined_events = sorted(scenario.events + target_events_aligned, 
                                 key=lambda x: x.time)

        enriched_log = enrich_log(combined_events)
        dump_vcd(enriched_log, f"scenario_{scenario.name}.vcd")

        return Log(enriched_log)

class HilConnector(ViaemsInterface):
    def __init__(self):
        self._target_reset()

        # find the ECU device via USB
        viaems_device = usb.core.find(idVendor=0x1209, idProduct=0x2041)
        if not viaems_device:
            raise ValueError("No Viaems USB device found")
        
        if viaems_device.is_kernel_driver_active(0):
            viaems_device.detach_kernel_driver(0)

        self.device = viaems_device
        self.read_buffer = b''

    def _target_reset(self):
        # Output 8 (MSB) is connected to nRST, pull it low and then high
        subprocess.run(
                [
                    "viaems-fpga-tb",
                    "--cmd-outputs", 
                    "00"
                ], check=True)
        time.sleep(0.1)
        subprocess.run(
                [
                    "viaems-fpga-tb",
                    "--cmd-outputs", 
                    "80"
                ], check=True)
        time.sleep(1)


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


    def log_from_capture(self, msgs: List[str]) -> List[CaptureEvent]:
        last_raw_value = 0

        result : List[CaptureEvent] = []
        for msg in msgs:
            # Capture time is 60 MHz, convert to 4 Mhz
            time = int(float(msg.rstrip().split()[2]) / 15)

            raw_value = int(msg.rstrip().split()[3], 16)

            # Trigger 1 low->high transition
            if (raw_value & (1 << 23)) and not (last_raw_value & (1 << 23)):
                result.append(CaptureTriggerEvent(time=time, trigger=1))

            # Trigger 0 low->high transition
            if (raw_value & (1 << 22)) and not (last_raw_value & (1 << 22)):
                result.append(CaptureTriggerEvent(time=time, trigger=0))

            # Values change
            if (raw_value & 0xffff) != (last_raw_value & 0xffff):
                result.append(CaptureOutputEvent(time=time, values=raw_value & 0xffff))

            # Gpios
            if (raw_value & 0x3f0000) != (last_raw_value & 0x3f0000):
                result.append(CaptureGpioEvent(time=time, values=(raw_value & 0x3f0000) >> 16))

            last_raw_value = raw_value

        return result

    def _render_target_inputs(self, scenario, file):
        adc_values = [0.0] * 16
        current_time = 0

        # Render into commands for the fpga test bench
        # Time counter is 15 MHz instead of the 4 MHz we are provided

        def _advance_to_time(time):
            nonlocal current_time
            MAX_DELAY = (2**25) - 1
            while time > current_time:
                delay = time - current_time - 1 # Delay takes one cycle
                delay = min(delay, MAX_DELAY)
                file.write(f"d {delay}\n")
                current_time += delay + 1

        def _check_time(evtime):
            nonlocal current_time
            if abs(evtime - current_time) > 4:
                raise Exception(f"Unable to render target commands without excessive error: {evtime} (vs actual {current_time})")


        for event in scenario.events:
            event_time = int((15.0 / 4.0) * event.time)

            match event:
                case SimToothEvent(trigger=trigger):
                    _advance_to_time(event_time)
                    pins = 0x81 if trigger == 0 else 0x82
                    # Create a high pulse of length 1 
                    _check_time(event_time)
                    file.write(f"o 0 {pins}\n")
                    file.write(f"o 0 {0x80}\n")
                    current_time += 2

                case SimADCEvent(values=values):
                    _advance_to_time(event_time)
                    for sel in range(8):
                        idx1 = sel * 2
                        idx2 = sel * 2 + 1
                        if (values[idx1] != adc_values[idx1]) or \
                           (values[idx2] != adc_values[idx2]):
                            _check_time(event_time)
                            val1 = int(values[idx1] / 5.0 * 4095)
                            val2 = int(values[idx2] / 5.0 * 4095)
                            file.write(f"a {sel} {val1} {val2}\n")
                            current_time += 1

                    adc_values = values

                case SimEndEvent():
                    _advance_to_time(event_time)

    

    def execute_scenario(self, scenario, settings=[]):
        self.set(["test", "event-logging"], True)
        for path, value in settings:
            self.set(path, value)

        tf = open(f"scenario_{scenario.name}.inputs", "w")
        self._render_target_inputs(scenario, tf)
        tf.close()

        # Start test bench
        tb_process = subprocess.Popen(
                [
                    "viaems-fpga-tb",
                    "--scenario", f"scenario_{scenario.name}.inputs",
                    "--output", f"scenario_{scenario.name}.outputs",
                    ], 
                )

        target_msgs = []
        while tb_process.poll() is None:
            try:
                msg = self.recv()
                target_msgs.append(msg)
            except EOFError:
                break

        tb_process.communicate()
        assert tb_process.returncode == 0
        self.kill()

        tb_file = open(f"scenario_{scenario.name}.outputs")
        tb_msgs = tb_file.readlines()
        tb_file.close()

        tb_events = self.log_from_capture(tb_msgs)
        target_events = log_from_target_messages(target_msgs)

        tb_events_aligned = align_triggers_to_sim(scenario.events, tb_events)

        target_events_aligned = align_triggers_to_sim(scenario.events, target_events)


        combined_events = sorted(scenario.events + \
                                 target_events_aligned + \
                                 tb_events_aligned, 
                                 key=lambda x: x.time)

        combined_events = filter(lambda x: x.time >= 0, combined_events)

        enriched_log = enrich_log(combined_events)
        dump_vcd(enriched_log, f"scenario_{scenario.name}.vcd")

        return Log(enriched_log)



