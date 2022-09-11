import unittest
from viaems import ViaemsWrapper, ticks_for_rpm_degrees
import sys
import cbor

class ToothEvent:
  def __init__(self, delay, trigger, angle=None):
    self.delay = delay
    self.angle = angle
    self.trigger = trigger

  def render(self):
    return [self.delay, self.trigger]

class EndEvent:
  def render(self):
    return [0, -1]

def clamp_angle(angle):
  if angle >= 720:
    angle -= 720
  if angle < 0:
    angle += 720
  return angle


class CrankNMinus1PlusCam_Wheel:
  def __init__(self, N):
    self.N = N
    self.degrees_per_tooth = 360.0 / N
    self.revolutions = 0
    self.index = 0

    # Populate even tooth wheel for trigger 0
    self.wheel = [(0, x * self.degrees_per_tooth) 
        for x in range(N * 2)
        # Except for a missing tooth each rev
        if x != 1 and x != (N + 1)
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
    if angle == 0 or angle == 360:
      self.revolutions += 1
#    return ToothEvent(delay=time, trigger=trigger, angle=angle)
    return (trigger, angle)

class Scenario:
  def __init__(self, name, decoder):
    self.name = name
    self.events = []
    self.decoder = decoder
    self.time = 0
    self.rpm = 0

    self._last_event = None
    self._last_event_time = None

  def _advance(self):
    delay = self.decoder.time_to_next_trigger(self.rpm)
    tooth, angle = self.decoder.next(self.rpm)

    previousdelay = 0
    # make sure to account for any delta due to waits
    if self._last_event_time:
      previousdelay = self.time - self._last_event_time
    ev = ToothEvent(delay=delay + previousdelay, trigger=tooth, angle=angle)

    self.events.append(ev)
    self.time += delay
    self._last_event = ev
    self._last_event_time = self.time

  def _try_next(self, ticks):
    next_delay = self.decoder.time_to_next_trigger(self.rpm)
    if self.time + ticks > self._last_event_time + next_delay:
      self._advance()
    else:
      self.time += ticks

  def set_rpm(self, rpm):
    self.rpm = rpm

  def wait_until_revolution(self, rev):
    if self.rpm == 0:
      return
    if not self._last_event:
      self._advance()

    while self.decoder.revolutions <= rev:
      self._try_next(self._last_event.delay)

  def wait_milliseconds(self, ms):
    targettime = self.time + 4000 * ms

    if self.rpm == 0:
      self.time += 4000 * ms;
      return

    if not self._last_event:
      self._advance()

    while self.time <= targettime:
      self._try_next(4000 * ms)

  def end(self):
    pass

  def cross_reference_resuls(self, results):
    return results


class NMinus1DecoderTests(unittest.TestCase):

    def setUp(self):
        self.conn = ViaemsWrapper("obj/hosted/viaems")

    def tearDown(self):
        self.conn.kill()

    def test_rpm_ramp(self):
      scenario = Scenario("rpm_ramp", CrankNMinus1PlusCam_Wheel(36))
      for i in range(1,100):
          scenario.set_rpm(i*60)
          scenario.wait_milliseconds(50)
      scenario.set_rpm(0)
      scenario.wait_milliseconds(1000)
      scenario.end()

      results = self.conn.execute_scenario(scenario)

#      enriched_results = scenario.cross_reference_results(results)
#      enriched_results.validate_events()
      

    def test_start_stop_start(self):
      scenario = Scenario("start_stop_start", CrankNMinus1PlusCam_Wheel(36))
      scenario.wait_milliseconds(1000)
      scenario.set_rpm(1000);
      scenario.wait_milliseconds(1000)
      scenario.set_rpm(0)
      scenario.wait_milliseconds(1000)
      scenario.set_rpm(1000);
      scenario.wait_milliseconds(1000)
      scenario.end()

      results = self.conn.execute_scenario(scenario)
      for r in results:
        print(r)

#      enriched_results = scenario.cross_reference_results(results)
#      enriched_results.validate_events()
      

if __name__ == "__main__":
    unittest.main()
