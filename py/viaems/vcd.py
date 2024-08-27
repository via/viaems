from vcd import VCDWriter


def _find_first_desc(events):
    for event in events:
        if event["type"] == "description":
            return event

    return None


def dump_vcd(events, file):
    current_time = 0
    current_time_wraps = 0

    desc = _find_first_desc(events)
    sensors = []
    with open(file, "w") as openfile:
        with VCDWriter(openfile, timescale="1 ns") as writer:
            trigger0 = writer.register_var("events", "trigger0", "event")
            trigger1 = writer.register_var("events", "trigger1", "event")
            outputs = writer.register_var("events", "outputs", "wire", size=16)
            gpios = writer.register_var("events", "gpios", "wire", size=8)
            if desc:
                for key in desc["keys"]:
                    var = writer.register_var("sensors", key, "real")
                    sensors.append(var)

            for event in events:
                if event["time"] < current_time:
                    current_time_wraps += 1
                current_time = event["time"]

                if event["type"] == "event" and event["event"]["type"] == "trigger":
                    if event["event"]["pin"] == 0:
                        writer.change(trigger0, current_time * 250, True)
                    if event["event"]["pin"] == 1:
                        writer.change(trigger1, current_time * 250, True)

                if event["type"] == "event" and event["event"]["type"] == "output":
                    writer.change(
                        outputs, current_time * 250, event["event"]["outputs"]
                    )

                if event["type"] == "event" and event["event"]["type"] == "gpio":
                    writer.change(gpios, current_time * 250, event["event"]["outputs"])

                if desc and event["type"] == "feed":
                    for i in range(len(sensors)):
                        writer.change(
                            sensors[i], current_time * 250, float(event["values"][i])
                        )
