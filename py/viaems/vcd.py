from vcd import VCDWriter

from viaems.events import *

def dump_vcd(log, file):
    current_time = 0

    first_feed = log.filter_feeds()[0]
    sensors = []

    with open(file, "w") as openfile:
        with VCDWriter(openfile, timescale="1 ns") as writer:
            trigger0 = writer.register_var("events", "trigger0", "event")
            trigger1 = writer.register_var("events", "trigger1", "event")
            outputs = writer.register_var("events", "outputs", "wire", size=16)
            gpios = writer.register_var("events", "gpios", "wire", size=8)
            marks = writer.register_var("events", "mark", "event")

            for key, value in first_feed.values.items():
                var = writer.register_var("sensors", key, "real")
                sensors.append(var)

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

                    case TargetFeedEvent(time, values):
                        for idx, name in enumerate(values.keys()):
                            writer.change(
                                    sensors[idx], time * 250,
                                    float(values[name])
                                    )

                    case SimMarkEvent(time, name):
                        writer.change(marks, time * 250, True)

