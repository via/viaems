import unittest
import sys

from viaems.decoder import CrankNMinus1PlusCam_Wheel
from viaems.scenario import Scenario
from viaems.testcase import TestCase
from viaems.util import ticks_for_rpm_degrees, ms_ticks
from viaems.validation import validate_outputs



class SafetyTests(TestCase):

    def test_fuel_pump_cutoff(self):
      scenario = Scenario("fuel_pump_cutoff", CrankNMinus1PlusCam_Wheel(36))
      scenario.set_brv(12.5)
      scenario.set_map(102);

      scenario.wait_milliseconds(100)
      # Fuel pump should be on
      t1 = scenario.mark()

      scenario.wait_milliseconds(5000)
      # Fuel pump should time out
      t2 = scenario.mark()

      scenario.set_rpm(300)
      scenario.wait_milliseconds(500)
      # Fuel pump should be on again
      t3 = scenario.mark()

      scenario.set_rpm(0)
      scenario.wait_milliseconds(500)
      # Fuel pump should still be on
      t4 = scenario.mark()
      
      scenario.wait_milliseconds(5000)
      # Fuel pump should still be off
      t5 = scenario.mark()

      scenario.end()

      results = self.conn.execute_scenario(scenario)

      assert results.filter_latest_gpio_at(t1).pin(0) == True
      assert results.filter_latest_gpio_at(t2).pin(0) == False
      assert results.filter_latest_gpio_at(t3).pin(0) == True
      assert results.filter_latest_gpio_at(t4).pin(0) == True
      assert results.filter_latest_gpio_at(t5).pin(0) == False

    def test_rev_limiter(self):
      settings = [
          (["decoder", "rpm-limit-start"], 3000),
          (["decoder", "rpm-limit-stop"], 4000),
          ]
      scenario = Scenario("rev_limiter", CrankNMinus1PlusCam_Wheel(36))
      t0 = scenario.mark()

      scenario.set_rpm(3800)
      scenario.set_brv(14.0)
      scenario.set_map(40);

      scenario.wait_milliseconds(500)
      # Engine should be running
      t1 = scenario.mark()

      # Raise rpms past limiter
      for rpm in range(3800, 4200, 10):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(10)

       # RPM limit should have taken effect
      t2 = scenario.mark()

      scenario.wait_milliseconds(100)

      # Raise rpms past limiter
      t3 = scenario.mark();
      for rpm in range(4200, 2800, -20):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(10)

      t4 = scenario.mark()

      scenario.wait_milliseconds(500)

      scenario.end()

      results = self.conn.execute_scenario(scenario, settings)

      t0_t1_outputs = results.filter_between(t0, t1).filter_enriched_outputs()
      assert len(t0_t1_outputs) > 0

      rpm_limit_activate_first_feed = next(filter(lambda i: i.values['rpm_cut'] == 1,
                               results.filter_between(t1, t2).filter_feeds())) 

      last_output = list(results.filter_between(t1, t2).filter_enriched_outputs())[-1]

      assert last_output.time - rpm_limit_activate_first_feed.time < 40000
      assert rpm_limit_activate_first_feed.values['rpm'] < 4100

      # TODO validate the comedown ramp

if __name__ == "__main__":
    if len(sys.argv) > 1:
        sys.argv = sys.argv[1:] # Pass commandline options to test
    unittest.main(argv=sys.argv)
