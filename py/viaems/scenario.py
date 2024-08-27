class ToothEvent:
    def __init__(self, time, delay, trigger, angle=None, rpm=None, cycle=None):
        self.time = time
        self.delay = delay
        self.angle = angle
        self.rpm = rpm
        self.trigger = trigger
        self.cycle = cycle

    def render(self):
        return f"t {int(self.delay)} {self.trigger}\n"


class EndEvent:
    def __init__(self, delay):
        self.delay = delay

    def render(self):
        return f"e {int(self.delay)}\n"


class AdcEvent:
    def __init__(self, delay, values):
        self.delay = delay
        self.values = [x for x in values]  # deep copy

    def render(self):
        vals = " ".join([str(x) for x in self.values])
        return f"a {int(self.delay)} {vals}\n"


class Scenario:
    def __init__(self, name, decoder):
        self.name = name
        self.events = []
        self.decoder = decoder
        self.time = 0
        self.rpm = 0
        self.adc = [0.0] * 16
        self.adc_sample_rate = 5000
        self.adc_delay = 4000000 / self.adc_sample_rate

        self._last_event = None
        self._last_trigger_time = 0
        self._last_adc_time = 0

    # TODO: factor these out, maybe parse a config to set them
    def set_brv(self, brv):
        self.adc[2] = brv / 24.5 * 5.0

    def set_map(self, map_kpa):
        self.adc[3] = (map_kpa - 12) / (420 - 12) * 5.0

    def set_iat(self, iat):
        pass

    def set_clt(self, clt):
        pass

    def set_clt(self, clt):
        pass

    def _advance(self, max_delay=None):
        # Advance to the next known point in time, which is either the next
        # trigger/tooth, or the next adc sample event

        if self.rpm == 0:
            next_trigger_time = 3600 * 4000000  # Far future
        else:
            next_trigger_time = (
                self._last_trigger_time + self.decoder.time_to_next_trigger(self.rpm)
            )
            # for when rpm becomes nonzero
            if next_trigger_time < self.time:
                next_trigger_time = self.time

        next_adc_time = self._last_adc_time + self.adc_delay
        if max_delay is not None:
            max_time = self.time + max_delay

            if max_time < next_adc_time and max_time < next_trigger_time:
                self.time += max_delay
                return

        if next_trigger_time < next_adc_time:
            tooth, angle = self.decoder.next(self.rpm)

            delay = next_trigger_time - self.time
            ev = ToothEvent(
                time=next_trigger_time,
                delay=delay,
                trigger=tooth,
                angle=angle,
                rpm=self.rpm,
                cycle=self.decoder.cycle,
            )

            self.events.append(ev)
            self.time += delay
            self._last_event = ev
            self._last_trigger_time = next_trigger_time
        else:
            delay = next_adc_time - self.time
            ev = AdcEvent(delay=delay, values=self.adc)

            self.events.append(ev)
            self.time += delay
            self._last_event = ev
            self._last_adc_time = next_adc_time

    def set_rpm(self, rpm):
        self.rpm = rpm

    def wait_until_revolution(self, rev):
        while self.decoder.revolutions <= rev:
            self._advance()

    def wait_milliseconds(self, ms):
        targettime = self.time + 4000 * ms

        while self.time <= targettime:
            self._advance(4000 * ms)

    def end(self, delay=1):
        ev = EndEvent(delay=delay * 4000000)
        self.events.append(ev)

    def now(self):
        return self.time
