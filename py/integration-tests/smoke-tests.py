import unittest
import sys
import cbor

from viaems.connector import ViaemsWrapper
from viaems.decoder import CrankNMinus1PlusCam_Wheel
from viaems.scenario import Scenario
from viaems.testcase import TestCase
from viaems.util import ticks_for_rpm_degrees, ms_ticks
from viaems.validation import validate_outputs



class NMinus1DecoderTests(TestCase):

    def test_start_stop_start(self):
      scenario = Scenario("start_stop_start", CrankNMinus1PlusCam_Wheel(36))
      scenario.set_brv(12.5)
      scenario.set_map(102);
      scenario.wait_milliseconds(1000)
      t1 = scenario.now()

      # simulate a crank condition
      scenario.set_rpm(300);
      scenario.set_brv(9.0)
      scenario.set_map(80.0)
      scenario.wait_milliseconds(1000)

      # engine catch ramp-up
      t2 = scenario.now()
      for rpm in range(300, 800, 100):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(100)

      t3 = scenario.now()

      scenario.set_map(35)
      scenario.set_brv(14.4)
      scenario.wait_milliseconds(1000)

      t4 = scenario.now()

      scenario.set_rpm(0);
      scenario.set_map(102)
      scenario.set_brv(12)
      scenario.wait_milliseconds(5000)

      t5 = scenario.now()
      scenario.end()

      results = self.conn.execute_scenario(scenario)

#     t1 - t2 is cranking. validate:
#       - we gain sync within 500 ms
#       - we get first output within 1 cycles
#       - (after sync) advance is reasonable
#       - (after sync) pw is reasonable
      
      first_sync = next(filter(lambda i: i.values['sync'] == 1,
                               results.filter_between(t1, t2).filter_feeds()))

      self.assertLessEqual(first_sync.time - t1, ms_ticks(500))

      for f in results.filter_between(first_sync.time + ms_ticks(10), t2).filter_feeds():
          # start looking 10 ms later, since actual calculations race with decoder
          self.assertWithin(f.values['advance'], 10, 20)
          self.assertEqual(f.values['sync'], 1)
          self.assertWithin(f.values['fuel_pulsewidth_us'], 4400, 4500)

      first_output = next(results.filter_between(t1, t2).filter_outputs())
      self.assertEqual(first_output.cycle, 1)

      
#    t2-t4:
#     - should have sync
#     - fully validate output
      for f in results.filter_between(t2, t4).filter_feeds():
          self.assertEqual(f.values['sync'], 1)

#    t4-t5:
#     - we lose sync within X ms, and it stays lost
#     - last output within Y ms
      last_sync = next(filter(lambda i: i.values['sync'] == 0,
                               results.filter_between(t4, t5).filter_feeds()))

      self.assertWithin(last_sync.time - t4, ms_ticks(1), ms_ticks(100))
      remaining_syncs = filter(lambda i: i.values['sync'] == 1,
                               results.filter_between(last_sync.time, t5).filter_feeds())
      self.assertEqual(len(list(remaining_syncs)), 0)

      outputs = list(results.filter_between(t4, t5).filter_outputs())
      if len(outputs) > 0:
          self.assertWithin(outputs[-1].time, ms_ticks(1), ms_ticks(100))

      # Finally, validate all the outputs are associated with an event and match
      # the expected angles/durations
      is_valid, msg = validate_outputs(results.filter_between(t1, t5))
      self.assertTrue(is_valid, msg)


if __name__ == "__main__":
    unittest.main()
