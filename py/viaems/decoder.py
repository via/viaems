from viaems.util import ticks_for_rpm_degrees, clamp_angle


class CrankNMinus1PlusCam_Wheel:
    def __init__(self, N, offset=0):
        self.N = N
        self.degrees_per_tooth = 360.0 / N
        self.cycle = 0
        self.index = 0
        self.offset = offset

        # Populate even tooth wheel for trigger 0
        self.wheel = [
            (0, x * self.degrees_per_tooth)
            for x in range(N * 2)
            # Except for a missing tooth each rev
            # The first tooth *after* the gap is the 0 degree mark, and we want
            # the first tooth to be angle 0
            if x != 35 and x != (N + 35)
        ]

        # Add a cam sync at 45 degrees
        self.wheel.append((1, 45.0))
        self.wheel.sort(key=lambda x: x[1])

    def _next_index(self):
        return (self.index + 1) % len(self.wheel)

    def time_to_next_trigger(self, rpm):
        current_angle = self.wheel[self.index][1]
        next_angle = self.wheel[self._next_index()][1]
        diff = clamp_angle(next_angle - current_angle)
        return ticks_for_rpm_degrees(rpm, diff)

    def next(self, rpm):
        self.index = self._next_index()
        trigger, angle = self.wheel[self.index]
        if angle == 0:
            self.cycle += 1
        return (trigger, clamp_angle(angle - self.offset))
