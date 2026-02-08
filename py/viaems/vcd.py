from vcd import VCDWriter

from viaems.events import *
from viaems_proto import console_pb2 as console

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

            for s in console.Sensors.DESCRIPTOR.fields:
                var = writer.register_var("sensors", s.name, "real")
                sensors[s.name] = var

            for c in console.Calculations.DESCRIPTOR.fields:
                var = writer.register_var("calculations", c.name, "real")
                calculations[c.name] = var

            for p in console.Position.DESCRIPTOR.fields:
                var = writer.register_var("position", p.name, "real")
                position[p.name] = var

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
                        if update.HasField('sensors'):
                            for desc, val in update.sensors.ListFields():
                                writer.change(sensors[desc.name], time * 250, val)
                        if update.HasField('position'):
                            for desc, val in update.position.ListFields():
                                writer.change(position[desc.name], time * 250, val)
                        if update.HasField('calculations'):
                            for desc, val in update.calculations.ListFields():
                                writer.change(calculations[desc.name], time * 250, val)
                    case SimMarkEvent(time, name):
                        writer.change(marks, time * 250, True)

