from typing import Union
import bisect

from viaems.util import degrees_for_tick_rpm, clamp_angle
from viaems.events import *

# Validation
# 
# Scenario Events in Scenario Time 
#
# Console Messages -> Event Log in Target Time
#
# Capture Events -> Event Log in Capture Time
#
# First, all three event logs are correlated using the trigger. This produces
# a combination log converted to Scenario Time
#
# Second, higher level output events are added to the log
#
# Lastly, the high level output events are validated with an expected config and
# surrounding feed update events
#
#



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



def enrich_log(log) -> Log:
    # Iterate through the log:
    #   - For each trigger event, match with next input trigger event. Keep the
    #   current time delta, keep the current rpm/angle/time
    #
    #   - For each output event, produce an OutputEvent enriched with
    #   angle/duration
    #
    #   - For each feed event, use the last kept description event to produce a
    #   FeedEvent

    rise_times = [None] * 16
    previous_outputs = [False] * 16
    last_trigger_input = None

    result = []

    for entry in log:
        result.append(entry)
        match entry:
            case SimToothEvent():
                last_trigger_input = entry

            case TargetOutputEvent() | CaptureOutputEvent():
                if last_trigger_input is None:
                    raise ValueError("Output event before first trigger")

                outputs = [ (entry.values & (1 << bit)) > 0 for bit in range(0, 16) ]
                for pin, (old, new) in enumerate(list(zip(previous_outputs, outputs))):
                    if old == False and new == True:
                        rise_times[pin] = entry.time
                    if new == False and old == True:
                        trigger_delta = entry.time - last_trigger_input.time
                        degs = degrees_for_tick_rpm(trigger_delta, last_trigger_input.rpm)
                        angle = clamp_angle(last_trigger_input.angle + degs)

                        oc = config.lookup(pin, angle)
                        advance = None if oc is None else -oc._offset(angle)

                        oe = EnrichedOutputEvent(
                            time=entry.time,
                            pin=pin,
                            duration_us=(entry.time - rise_times[pin]) / 4,
                            end_angle=angle,
                            advance=advance,
                            cycle=last_trigger_input.cycle,
                            config=oc,
                        )
                        result.append(oe)

                previous_outputs = outputs

    return Log(result)


def validate_outputs(log: Log) -> (bool, str):
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
    current_cputime = None

    for o in log:
        match o:
            case TargetFeedEvent(values=values):
                current_fuel_pw = values["fuel_pulsewidth_us"]
                current_ign_pw = values["dwell"]
                current_ign_adv = values["advance"]
                current_cputime = values["cputime"]

            case EnrichedOutputEvent():
                if o.config is None:
                    return False, f"Output not associated with configuration: {o}"

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

                if o.config.typ == OutputConfig.FUEL and current_fuel_pw is not None:
                    if abs(o.duration_us - current_fuel_pw) > 5:
                        return (
                            False,
                            f"Fuel output {o.pin} at time {o.time} is duration "
                            + f"{o.duration_us}, expected {current_fuel_pw}",
                        )
                if o.config.typ == OutputConfig.IGN and current_ign_pw is not None:
                    if abs(o.duration_us - current_ign_pw) > 500:
                        return (
                            False,
                            f"Ignition output {o.pin} at time {o.time} is duration "
                            + f"{o.duration_us}, expected {current_ign_pw}",
                        )
                    if abs(o.advance - current_ign_adv) > 2:
                        return (
                            False,
                            f"Ignition output at time {o.time} ({current_cputime}) is at advance "
                            + f"{o.advance}, expected {current_ign_adv}",
                        )

                current_cycle_outputs += 1

    expected_count = len(config.outputs)

    if len(cycles) < 3:
        return False, "fewer than 3 cycles of outputs to validate"
    if cycles[0] == 0 or cycles[0] > expected_count:
        return False, f"cycle 0 bad event count: {cycles[0]}"
    if cycles[-1] == 0 or cycles[-1] > expected_count:
        return False, f"cycle {len(cycles) - 1} bad event count"
    for idx, val in enumerate(cycles[1:-1]):
        if val != len(config.outputs):
            return False, f"cycle {idx} missing outputs"

    return True, None
