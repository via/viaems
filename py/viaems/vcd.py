from vcd import VCDWriter
from betterproto2 import OutputFormat, Casing

from viaems.events import *
from viaems_proto.viaems import console

def dump_vcd(log, file):
    current_time = 0

    first_update = log.filter_updates()[0]
    sensors = {}
    calculations = {}
    position = {}

    with open(file, "w") as openfile:
        with VCDWriter(openfile, timescale="1 ns") as writer:
            trigger0 = writer.register_var("events", "trigger0", "event")
            trigger1 = writer.register_var("events", "trigger1", "event")
            outputs = writer.register_var("events", "outputs", "wire", size=16)
            gpios = writer.register_var("events", "gpios", "wire", size=8)
            marks = writer.register_var("events", "mark", "event")

            for s in console.Sensors.__dataclass_fields__.keys():
                var = writer.register_var("sensors", s, "real")
                sensors[s] = var

            for s in console.Calculations.__dataclass_fields__.keys():
                var = writer.register_var("calculations", s, "real")
                calculations[s] = var

            for s in console.Position.__dataclass_fields__.keys():
                var = writer.register_var("position", s, "real")
                position[s] = var

            for event in log:
                match event:
                    case SimToothEvent(time, trigger):
                        if trigger == 0:
                            writer.change(trigger0, time * 250, True)
                        if trigger == 1:
                            writer.change(trigger1, time * 250, True)

                    case TargetOutputEvent(time, values):
                        writer.change(outputs, time * 250, values)

                    case CaptureOutputEvent(time, values):
                        writer.change(outputs, time * 250, values)

                    case TargetGpioEvent(time, values):
                        writer.change(gpios, time * 250, values)

                    case CaptureGpioEvent(time, values):
                        writer.change(gpios, time * 250, values)

                    case TargetEngineUpdateEvent(time, update):
                        if update.is_set('sensors'):
                            for s, w in sensors.items():
                                writer.change(w, time * 250, float(getattr(update.sensors, s)))
                        if update.is_set('position'):
                            for s, w in position.items():
                                writer.change(w, time * 250, float(getattr(update.position, s)))
                        if update.is_set('calculations'):
                            for s, w in calculations.items():
                                writer.change(w, time * 250, float(getattr(update.calculations, s)))
                    case SimMarkEvent(time, name):
                        writer.change(marks, time * 250, True)

