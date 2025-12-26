from dataclasses import dataclass
from typing import Dict, List
from copy import copy

CaptureTime = int
ScenarioTime = int
TargetTime = int

Time = ScenarioTime | TargetTime | CaptureTime

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


@dataclass
class Event:
    time: Time

class TargetEvent(Event):
    pass

class SimEvent(Event):
    pass

class CaptureEvent(Event):
    pass

@dataclass
class SimMarkEvent(SimEvent):
    name: str

@dataclass
class SimEndEvent(SimEvent):
    pass

@dataclass
class SimToothEvent(SimEvent):
    trigger: int
    rpm: int
    angle: float
    cycle: int

@dataclass
class SimADCEvent(SimEvent):
    values: List[float]

@dataclass
class TargetFeedEvent(TargetEvent):
    values: Dict[str, float]

@dataclass
class TargetTriggerEvent(TargetEvent):
    trigger: int

@dataclass
class TargetOutputEvent(TargetEvent):
    values: int

@dataclass
class TargetGpioEvent(TargetEvent):
    values: int
    def pin(self, pin):
        return (self.values & (1 << pin)) > 0

@dataclass
class CaptureTriggerEvent(TargetEvent):
    trigger: int

@dataclass
class CaptureOutputEvent(CaptureEvent):
    values: int

@dataclass
class CaptureGpioEvent(CaptureEvent):
    values: int
    def pin(self, pin):
        return (self.values & (1 << pin)) > 0

@dataclass
class EnrichedOutputEvent(Event):
        pin: int
        duration_us: float
        end_angle: float
        advance: None | float
        cycle: int
        config: OutputConfig

class Log(List[Event]):
    def filter_feeds(self):
        return Log(filter(lambda i: isinstance(i, TargetFeedEvent), self))

    def filter_outputs(self):
        return Log(filter(lambda i: isinstance(i, TargetOutputEvent), self))

    def filter_enriched_outputs(self):
        return Log(filter(lambda i: isinstance(i, EnrichedOutputEvent), self))

    def filter_after(self, time: ScenarioTime):
        return Log(filter(lambda i: i.time >= time, self))

    def filter_between(self, start: ScenarioTime, end: ScenarioTime):
        return Log(
            filter(lambda i: i.time >= start and i.time <= end, self)
        )

    def filter_gpios(self):
        return Log(filter(lambda i: isinstance(i, TargetGpioEvent) or isinstance(i, CaptureGpioEvent), self))

    def filter_latest_gpio_at(self, time) -> TargetGpioEvent | CaptureGpioEvent:
        gpio_events = self.filter_between(0, time).filter_gpios()
        return gpio_events[-1]

def log_from_target_messages(msgs: List[Dict]) -> List[TargetEvent]:
    desc_msg = next(filter(lambda m: m["type"] == "description", msgs))
    desc_keys = desc_msg["keys"]

    event_seq = None
    def check_seq(msg):
        nonlocal event_seq
        if event_seq is not None:
            if msg['seq'] != event_seq + 1:
                print(event_seq, " ", msg)
                raise ValueError("Skipped target event sequence number")
        event_seq = msg['seq']

    result : List[TargetEvent] = []
    for msg in msgs:
        time = int(msg["time"])
        if msg["type"] == "event":
            check_seq(msg)

        if msg["type"] == "feed":
            values = dict(zip(desc_keys, msg["values"]))
            result.append(TargetFeedEvent(time=TargetTime(time), values=values))
        elif msg["type"] == "event" and msg["event"]["type"] == "trigger": 
            result.append(TargetTriggerEvent(time=TargetTime(time),
                                             trigger=msg["event"]["pin"]))
        elif msg["type"] == "event" and msg["event"]["type"] == "output":
            result.append(TargetOutputEvent(time=TargetTime(time),
                                            values=msg["event"]["outputs"]))
        elif msg["type"] == "event" and msg["event"]["type"] == "gpio":
            result.append(TargetGpioEvent(time=TargetTime(time),
                                            values=msg["event"]["outputs"]))

    return result


def align_triggers_to_sim(simlog: List[SimEvent], other: List[TargetEvent]):
    sim_triggers = list(filter(lambda x: isinstance(x, SimToothEvent), simlog))
    other_triggers = list(filter(lambda x: isinstance(x, TargetTriggerEvent) or
                                 isinstance(x, CaptureTriggerEvent), other))

    if len(sim_triggers) < 2 or (len(sim_triggers) != len(other_triggers)):
        raise ValueError(f"Not enough triggers to align: {len(sim_triggers)} from scenario, {len(other_triggers)} from target")

    time_ratio = (other_triggers[-1].time - other_triggers[0].time) / (sim_triggers[-1].time - sim_triggers[0].time)
    ppm = (1.0 - time_ratio) * 1e6
    if abs(ppm) > 150:
        raise ValueError(f"Trigger time offsets vary: {ppm} ppm")

    delta = other_triggers[0].time - sim_triggers[0].time
    result = []
    for e in other:
         changed_ev = copy(e)
         match changed_ev.time:
             case TargetTime(time):
                 changed_ev.time = ScenarioTime((time - delta) * time_ratio)
             case CaptureTime(time):
                 changed_ev.time = ScenarioTime(time - delta)
         result.append(changed_ev)

    return result
