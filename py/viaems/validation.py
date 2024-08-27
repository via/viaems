from typing import Union

from viaems.util import degrees_for_tick_rpm, clamp_angle


class EnrichedLog:
    def __init__(self, log):
        self.log = log

    def filter_feeds(self):
        return EnrichedLog(filter(lambda i: type(i).__name__ == "FeedEvent", self.log))

    def filter_outputs(self):
        return EnrichedLog(
            filter(lambda i: type(i).__name__ == "OutputEvent", self.log)
        )

    def filter_after(self, time):
        return EnrichedLog(filter(lambda i: i.time >= time, self.log))

    def filter_between(self, start, end):
        return EnrichedLog(
            filter(lambda i: i.time >= start and i.time <= end, self.log)
        )

    def __next__(self):
        return next(self.log)

    def __iter__(self):
        return self


class OutputConfig:
    FUEL = 1
    IGN = 2

    def __init__(self, pin, typ, angle):
        self.pin = pin
        self.typ = typ
        self.angle = angle

    def _offset(self, angle):
        adv = angle - self.angle

        # normalize to (-360, 360) for easy comparison with bounds
        if adv >= 360:
            adv -= 720
        if adv <= -360:
            adv += 720
        return adv


class Config:
    def __init__(self):
        self.outputs = [
            OutputConfig(pin=0, typ=OutputConfig.IGN, angle=0),
            OutputConfig(pin=1, typ=OutputConfig.IGN, angle=120),
            OutputConfig(pin=2, typ=OutputConfig.IGN, angle=240),
            OutputConfig(pin=0, typ=OutputConfig.IGN, angle=360),
            OutputConfig(pin=1, typ=OutputConfig.IGN, angle=480),
            OutputConfig(pin=2, typ=OutputConfig.IGN, angle=600),
            OutputConfig(pin=8, typ=OutputConfig.FUEL, angle=700),
            OutputConfig(pin=9, typ=OutputConfig.FUEL, angle=460),
            OutputConfig(pin=10, typ=OutputConfig.FUEL, angle=220),
        ]

    def _offset_within(self, oc, angle, lower, upper):
        """Validate that the advance is within bounds. lower and upper must
        both be within (-360, 360)."""

        adv = oc._offset(angle)
        if adv >= lower and adv <= upper:
            return True
        return False

    def lookup(self, pin, end_angle):
        # TODO maybe don't hardcode this, but until then:
        #  - assume fuel is +/- 10 degrees
        #  - assume ignition is -50 to +10 degrees
        for oc in self.outputs:
            if pin != oc.pin:
                continue

            if oc.typ == OutputConfig.IGN:
                if self._offset_within(oc, end_angle, -50, 10):
                    return oc

            if oc.typ == OutputConfig.FUEL:
                if self._offset_within(oc, end_angle, -10, 10):
                    return oc


config = Config()


class OutputEvent:
    def __init__(self, time, pin, duration_us, end_angle, cycle):
        self.time = time
        self.pin = pin
        self.duration_us = duration_us
        self.end_angle = end_angle
        self.cycle = cycle
        self.oc = config.lookup(pin, end_angle)
        if self.oc:
            self.advance = -self.oc._offset(end_angle)


class FeedEvent:

    def __init__(self, time, keys, values):
        self.time = time
        self.values = dict(zip(keys, values))


def enrich_log(inputs, log) -> EnrichedLog:
    # Iterate through the log:
    #   - For each trigger event, match with next input trigger event. Keep the
    #   current time delta, keep the current rpm/angle/time
    #
    #   - For each output event, produce an OutputEvent enriched with
    #   angle/duration
    #
    #   - For each feed event, use the last kept description event to produce a
    #   FeedEvent

    triggers = filter(lambda i: type(i).__name__ == "ToothEvent", inputs)

    desc_fields = []
    last_trigger_input = None
    input_to_log_time_offset = 0
    rise_times = [None] * 16
    previous_outputs = [False] * 16

    result = []

    for entry in log:
        if entry["type"] == "event" and entry["event"]["type"] == "trigger":

            # Get next input trigger
            last_trigger_input = next(triggers)
            input_to_log_time_offset = entry["time"] - last_trigger_input.time

        if entry["type"] == "event" and entry["event"]["type"] == "output":
            outputs = [
                (entry["event"]["outputs"] & (1 << bit)) > 0 for bit in range(0, 16)
            ]
            for pin, (old, new) in enumerate(list(zip(previous_outputs, outputs))):
                if old == False and new == True:
                    rise_times[pin] = entry["time"]
                if new == False and old == True:
                    trigger_delta = entry["time"] - last_trigger_input.time
                    degs = degrees_for_tick_rpm(trigger_delta, last_trigger_input.rpm)
                    angle = clamp_angle(last_trigger_input.angle + degs)
                    oe = OutputEvent(
                        time=entry["time"] - input_to_log_time_offset,
                        pin=pin,
                        duration_us=(entry["time"] - rise_times[pin]) / 4,
                        end_angle=angle,
                        cycle=last_trigger_input.cycle,
                    )
                    result.append(oe)

            previous_outputs = outputs

        if entry["type"] == "description":
            desc_fields = entry["keys"]

        if entry["type"] == "feed":
            if len(entry["values"]) == len(desc_fields):
                # FeedEvent
                result.append(FeedEvent(entry["time"], desc_fields, entry["values"]))

    return EnrichedLog(result)


def validate_outputs(log):
    """Validate that all outputs are associated with a configured output,
    and that there are no gaps or missing outputs.  Each cycle should have
    the full count of configured outputs, except for the first and last."""

    is_first_cycle = True
    current_cycle = None
    current_cycle_outputs = 0
    cycles = []

    current_fuel_pw = None
    current_ign_pw = None
    current_ign_adv = None

    for o in log:
        if type(o).__name__ == "FeedEvent":
            current_fuel_pw = o.values["fuel_pulsewidth_us"]
            current_ign_pw = o.values["dwell"]
            current_ign_adv = o.values["advance"]
            continue

        if o.oc is None:
            return False, "Output not associated with configuration"

        if is_first_cycle:
            is_first_cycle = False
            current_cycle = o.cycle
        if o.cycle != current_cycle:
            if o.cycle > current_cycle + 1:
                # We skipped a cycle
                return False, "full cycle occured without outputs"
            cycles.append(current_cycle_outputs)
            current_cycle_outputs = 0
            current_cycle = o.cycle

        if o.oc.typ == OutputConfig.FUEL and current_fuel_pw is not None:
            if abs(o.duration_us - current_fuel_pw) > 5:
                return (
                    False,
                    f"Fuel output at time {o.time} is duration "
                    + f"{o.duration_us}, expected {current_fuel_pw}",
                )
        if o.oc.typ == OutputConfig.IGN and current_ign_pw is not None:
            if abs(o.duration_us - current_ign_pw) > 500:
                return (
                    False,
                    f"Ignition output at time {o.time} is duration "
                    + f"{o.duration_us}, expected {current_ign_pw}",
                )
            if abs(o.advance - current_ign_adv) > 2:
                return (
                    False,
                    f"Ignition output at time {o.time} is at advance "
                    + f"{o.advance}, expected {current_ign_adv}",
                )

        current_cycle_outputs += 1

    expected_count = len(config.outputs)

    if len(cycles) < 3:
        return False, "fewer than 3 cycles of outputs to validate"
    if cycles[0] == 0 or cycles[0] > expected_count:
        return False, "cycle 0 bad event count"
    if cycles[-1] == 0 or cycles[-1] > expected_count:
        return False, f"cycle {len(cycles) - 1} bad event count"
    for idx, val in enumerate(cycles[1:-1]):
        if val != len(config.outputs):
            return False, f"cycle {idx} missing outputs"

    return True, None
