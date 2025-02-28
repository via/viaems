from viaems.events import *

class Scenario:
    def __init__(self, name, decoder):
        self.name = name
        self.events = []
        self.decoder = decoder
        self.time = 0
        self.rpm = 0

        self.adc = [0.0] * 16
        self.adc[11] = 2.5
        self.adc_sample_rate = 5000
        self.adc_delay = 4000000 / self.adc_sample_rate

        self._last_trigger_time = 0

    # TODO: factor these out, maybe parse a config to set them
    def set_brv(self, brv):
        self.adc[2] = brv / 24.5 * 5.0
        self._emit_adc_event();

    def set_map(self, map_kpa):
        self.adc[3] = (map_kpa - 12) / (420 - 12) * 5.0
        self._emit_adc_event();

    def set_iat(self, iat):
        pass

    def set_clt(self, clt):
        pass

    def _emit_adc_event(self):
        self.events.append(
                SimADCEvent(
                    time=self.time, 
                    values=[v for v in self.adc]
                    ))


    def _advance(self, max_delay=None):
        # Advance the simulation. Will return after the next tooth event, or if
        # max_delay is met -- whichever is first

        if self.rpm == 0:
            # No ticks, adjust time and return
            self.time += max_delay
            return max_delay

        next_trigger_time = (
            self._last_trigger_time + self.decoder.time_to_next_trigger(self.rpm)
        )

        # for when rpm becomes nonzero
        if next_trigger_time < self.time:
            next_trigger_time = self.time

        if max_delay is not None:
            max_time = self.time + max_delay

            if max_time < next_trigger_time:
                self.time += max_delay
                return max_delay

        tooth, angle = self.decoder.next(self.rpm)

        ev = SimToothEvent(
            time=next_trigger_time,
            trigger=tooth,
            angle=angle,
            rpm=self.rpm,
            cycle=self.decoder.cycle,
        )

        self.events.append(ev)
        self.time = next_trigger_time
        self._last_trigger_time = next_trigger_time

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
        self.time += 4000000 * delay;
        self.events.append(SimEndEvent(self.time))

    def now(self):
        return self.time

    def mark(self, name=""):
        self.events.append(SimMarkEvent(self.time, name))
        return self.time

