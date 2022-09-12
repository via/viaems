import cbor
import json
import subprocess
import tempfile
import random
import os

from vcd import dump_log_to_vcd

TICKRATE = 4000000.0

def ticks_for_rpm_degrees(rpm, degrees):
  ticks_per_degree = (4000000 / 6.0) / rpm
  return int(degrees * ticks_per_degree)

def ms_ticks(ms):
  return 4000 * ms

class ViaemsWrapper:
    def __init__(self, binary):
        self.binary = binary
        self.process = None

    def start(self, replay=None):
        args = [self.binary]
        if replay:
          args.append("-i")
          args.append(replay)

        self.process = subprocess.Popen(args, bufsize=-1,
                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL)

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

    def recv_until_id(self, id, max_messages=100000):
        while True:
            result = cbor.load(self.process.stdout)
            if 'id' in result and int(result['id']) == id:
                return result
            max_messages -= 1
            if max_messages == 0:
                return None

    def structure(self):
        id = random.randint(0, 1024)
        self.send({
            "id": id,
            "type": "request",
            "method": "structure",
            })
        result = self.recv_until_id(id)
        return result

    def get(self, path):
        id = random.randint(0, 1024)
        self.send({
            "id": id,
            "type": "request",
            "method": "get",
            "path": path,
            })
        result = self.recv_until_id(id)
        return result

    def set(self, path, value):
        id = random.randint(0, 1024)
        self.send({
            "id": id,
            "type": "request",
            "method": "set",
            "path": path,
            "value": value,
            })
        result = self.recv_until_id(id)
        return result

    def execute_scenario(self, scenario):
      tf = open(f"scenario_{scenario.name}.inputs", "w")
      payload = {
          "events": []
          }
      for ev in scenario.events:
        d, t = ev.render()
        payload["events"].append(ev.render())
        tf.write(f"{d} {t}\n")

      self.start(replay=tf.name)
      results = []
      while True:
        try:
          msg = self.recv()
          results.append(msg)
        except EOFError:
          break

      results.sort(key=lambda x: x["time"])
      with open(f"scenario_{scenario.name}.log", "w") as output:
        json.dump(results, output)
      with open(f"scenario_{scenario.name}.vcd", "w") as vcdfile:
        dump_log_to_vcd(results, vcdfile)
      return results


      




