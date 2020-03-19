#!/usr/bin/env python
import sys

"""
# TRIGGER0 1264275
# OUTPUTS 1264652 202
# TRIGGER0 1265051
# OUTPUTS 1265367 200
# OUTPUTS 1265678 204
# TRIGGER0 1265827
# OUTPUTS 1266355  4
# TRIGGER0 1266602
# TRIGGER0 1267376
# OUTPUTS 1267743 404
# TRIGGER0 1268149
# OUTPUTS 1268463 400
# OUTPUTS 1268761 401
# TRIGGER0 1268921
# OUTPUTS 1269446  1
# TRIGGER0 1269692
# TRIGGER0 1270462
# OUTPUTS 1270820 101
# TRIGGER0 1271231
# OUTPUTS 1271544 100
# OUTPUTS 1271828 102
# TRIGGER0 1271999
# OUTPUTS 1272523  2
# TRIGGER0 1272766
# TRIGGER0 1273532
# OUTPUTS 1273879 202
# TRIGGER0 1274298
# OUTPUTS 1274608 200
# OUTPUTS 1274877 204
# TRIGGER0 1275063
# OUTPUTS 1275582  4
"""

TICKRATE = 16000000

config = {
        "pins": {
            "0": {"type": "ignition", "angles": [0.0, 360.0]},
            "1": {"type": "ignition", "angles": [120.0, 480.0]},
            "2": {"type": "ignition", "angles": [240.0, 600.0]},
            "8": {"type": "fuel", "angles": [0.0, 360.0]},
            "9": {"type": "fuel", "angles": [120.0, 480.0]},
            "10": {"type": "fuel", "angles": [240.0, 600.0]},
            }
        }

def _wrap_angle(angle):
    while angle < 0:
        angle += 720.0
    while angle > 720.0:
        angle -= 720.0
    return angle


class Decoder:
    def __init__(self, triggers=24.0, offset=50.0):
        self.triggers = triggers
        self.offset = offset
        self.triggers_since_sync = 0
        self.last_trigger_time = 0.0
        self.last_trigger_angle = 0.0
        self.rpm = 0.0
        self.synced = False

    def trigger(self, time):
        if time == 0:
            return
        self.triggers_since_sync += 1
        moved_angle = 720.0 / self.triggers
        self.rpm = 60.0 / ((self.triggers / 2.0) * ((time - self.last_trigger_time) / TICKRATE))


        self.last_trigger_time = time
        self.last_trigger_angle = (self.last_trigger_angle + moved_angle) % 720
        if self.triggers_since_sync > self.triggers:
            self.synced = False

    def sync(self, time):
        if self.triggers_since_sync == self.triggers:
            self.synced = True
        else:
            self.synced = False

        self.last_trigger_angle = 0.0
        self.triggers_since_sync = 0

    def angle(self, time):
        passed = time - self.last_trigger_time
        passed_degrees = self.rpm / 60.0 * 360.0 * (passed / TICKRATE)
        return (-self.offset + self.last_trigger_angle + passed_degrees) % 720.0

class Validator:
    def __init__(self, pins):
        self.rev = 0
        self.pins = pins
        self.events_for_last_rev = 0
        self.last_pw = 0
        self.last_dwell = 0
        self.last_advance = 0

    def new_rev(self):
        self.rev += 1
        self.evcount = sum([len(config["pins"][x]["angles"]) for x in config["pins"]])
        if self.events_for_last_rev != self.evcount:
            print("Bad event count, rev {}".format(self.rev))
        self.events_for_last_rev = 0

    def _validate_ignition(self, time, pin, duration, angle):
        for possible_angle in pin["angles"]:
            adv = possible_angle - angle
            if (adv < 60 and adv > -10) or ((adv + 720) % 720 < 60 and (adv + 720) % 720 > -10):
                # This pin works out
                advance = _wrap_angle(adv)
                print("{}: ignition at angle {}, advance={} dwell={}".format(time,
                        _wrap_angle(angle), advance, duration))
                if (abs(duration - self.last_dwell) > 0.3):
                    print("Dwell changed rapidly")
                if (abs(advance - self.last_advance) > 1):
                    print("Advance changed rapidly")
                self.last_dwell = duration
                self.last_advance = advance
                return True
        print("Bad angle? angle {}, duration {} pin {}".format(angle,
            duration, pin))

    def _validate_fueling(self, time, pin, duration, angle):
        for possible_angle in pin["angles"]:
            adv = possible_angle - angle
            if (adv < 30 and adv > -30) or ((adv + 720) % 720 < 30 and (adv + 720) % 720 > -30):
                # This pin works out
                print("{}: fueling at angle {}, advance={} pw={}".format(time,
                        _wrap_angle(angle), _wrap_angle(adv), duration))
                if (abs(duration - self.last_pw) > 0.3):
                    print("Fueling changed rapidly")
                self.last_pw = duration
                return True
        print("Bad angle? angle {}, duration {} pin {}".format(angle,
            duration, pin))


    def event_ended(self, time, output, duration, angle):
        print("rev {} output {} duration: {}, angle: {}".format(
            self.rev, output, duration, angle))
        pin = self.pins[str(output)]
        if pin["type"] == "ignition":
            success = self._validate_ignition(time, pin, duration / (TICKRATE / 1000), angle)
            if success:
                self.events_for_last_rev += 1
        elif pin["type"] == "fuel": 
            success = self._validate_fueling(time, pin, duration / (TICKRATE / 1000), angle)
            if success:
                self.events_for_last_rev += 1

class Outputs:
    def __init__(self):
        self.outputs = [0] * 16
        self.last_times = [0] * 16
        self.rev = 0

    def changed(self, validator, time, value, angle):
        for idx, cur in enumerate(self.outputs):
            new = (value & (1 << idx) != 0)
            if new != cur:
                if new == False:
                    validator.event_ended(time, idx, time - self.last_times[idx],
                            angle)
                self.outputs[idx] = new
                self.last_times[idx] = time


if  __name__ == "__main__":
    decoder = Decoder()    
    outputs = Outputs()
    validator = Validator(config["pins"])
    for line in sys.stdin:
        if line.startswith("# TRIGGER0 "):
            time = float(line.split()[2])
            decoder.trigger(time)
        elif line.startswith("# TRIGGER1 "):
            time = float(line.split()[2])
            decoder.sync(time)
            validator.new_rev()
	elif line.startswith("# OUTPUTS"):
            time = float(line.split()[2])
            value = int(line.split()[3], 16)
            outputs.changed(validator, time, value, decoder.angle(time))
        print(line.rstrip())

  
