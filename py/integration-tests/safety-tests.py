import unittest
import sys

from viaems.decoder import CrankNMinus1PlusCam_Wheel
from viaems.scenario import Scenario
from viaems.testcase import TestCase
from viaems.util import ticks_for_rpm_degrees, ms_ticks
from viaems.validation import validate_outputs
from viaems.events import OutputConfig
from viaems_proto.viaems import console

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
      config = console.Configuration(
              rpm_cut=console.ConfigurationRpmCut(
                rpm_limit_start=3000,
                rpm_limit_stop=4000,
                )
              )
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

      # Lower rpms past "start" limiter
      t3 = scenario.mark();
      for rpm in range(4200, 2800, -20):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(10)

      scenario.wait_milliseconds(500)

      t4 = scenario.mark()

      scenario.end()

      results = self.conn.execute_scenario(scenario, config)

      t0_t1_outputs = results.filter_between(t0, t1).filter_enriched_outputs()
      assert len(t0_t1_outputs) > 0

      rpm_limit_activate_first_update = next(filter(lambda i: i.update.calculations.rpm_limit_cut == True,
                               results.filter_between(t1, t2).filter_updates())) 

      last_output = list(results.filter_between(t1, t2).filter_enriched_outputs())[-1]

      assert last_output.time - rpm_limit_activate_first_update.time < 40000
      assert rpm_limit_activate_first_update.update.position.average_rpm < 4100

      t3_t4_outputs = results.filter_between(t3, t4).filter_enriched_outputs()
      assert len(t3_t4_outputs) > 0

      rpm_limit_deactivate_first_update = next(filter(lambda i: i.update.calculations.rpm_limit_cut == False,
                               results.filter_between(t3, t4).filter_updates())) 

      first_output = list(results.filter_between(t3, t4).filter_enriched_outputs())[0]

      assert first_output.time - rpm_limit_deactivate_first_update.time < 40000
      assert rpm_limit_deactivate_first_update.update.position.average_rpm > 2900 and \
             rpm_limit_deactivate_first_update.update.position.average_rpm < 3100

      # Validate that the events going into the cut and out of the cut are valid
      is_valid, msg = validate_outputs(results.filter_between(t1, last_output.time))
      self.assertTrue(is_valid, msg)

      is_valid, msg = validate_outputs(results.filter_between(rpm_limit_deactivate_first_update.time, t4))
      self.assertTrue(is_valid, msg)

    def test_fuel_overduty(self):

      config = console.Configuration(
                   fueling=console.ConfigurationFueling(
                       injector_dead_time=console.ConfigurationTable1D(
                           data=console.ConfigurationTableRow(
                               values = [20.0] * 16,
                           )
                       )
                   ),
                   rpm_cut=console.ConfigurationRpmCut(
                       rpm_limit_start=3000,
                       rpm_limit_stop=4000,
                   )
               )

      scenario = Scenario("fuel_overduty", CrankNMinus1PlusCam_Wheel(36))
      scenario.set_brv(14.0)
      scenario.set_map(90);
      scenario.set_rpm(500)
      scenario.wait_milliseconds(1000)
      t1 = scenario.mark()

      for rpm in range(500, 6000, 10):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(10)
      scenario.wait_milliseconds(1000)

      t4 = scenario.mark()
      scenario.wait_milliseconds(500)
      scenario.end()

      results = self.conn.execute_scenario(scenario, config)

      is_valid, msg = validate_outputs(results.filter_between(t1, t4))
      self.assertTrue(is_valid, msg)

      first_fuel_cut = next(filter(lambda i: i.update.calculations.fuel_overduty_cut == True,
                               results.filter_between(t1, t4).filter_updates())) 

      for o in results.filter_between(first_fuel_cut.time,
                                      t4).filter_enriched_outputs():
          self.assertTrue(o.time - first_fuel_cut.time < ms_ticks(30))

      last_fuels = [None, None, None]
      for o in results.filter_between(t1, t4).filter_enriched_outputs():
          if o.config.typ != OutputConfig.FUEL:
              continue

          idx = o.pin - 8
          if last_fuels[idx] is not None:
              cycletime = o.time - last_fuels[idx].time 
              duty = 4 * o.duration_us / cycletime
              self.assertTrue(duty < 0.97, msg="Fuel Duty exceeds 97%")

          last_fuels[idx] = o
          



    def test_ignition_overduty(self):
      config = console.Configuration(
                 ignition=console.ConfigurationIgnition(
                   min_dwell_us=6000,
                   dwell=console.ConfigurationTable1D(
                     data=console.ConfigurationTableRow(
                       values = [9.0] * 4
                     )
                   )
                 ),
              rpm_cut=console.ConfigurationRpmCut(
                rpm_limit_start=11000,
                rpm_limit_stop=10000,
                )
              )

      scenario = Scenario("dwell_overduty", CrankNMinus1PlusCam_Wheel(36))
      scenario.set_brv(14.0)
      scenario.set_map(90);
      scenario.set_rpm(500)
      scenario.wait_milliseconds(1000)
      t1 = scenario.mark()

      for rpm in range(500, 10000, 10):
          scenario.set_rpm(rpm)
          scenario.wait_milliseconds(10)
      scenario.wait_milliseconds(1000)

      t4 = scenario.mark()
      scenario.wait_milliseconds(500)
      scenario.end()

      results = self.conn.execute_scenario(scenario, config)

      is_valid, msg = validate_outputs(results.filter_between(t1, t4))
      self.assertTrue(is_valid, msg)

      # check that dwell starts to be corrected below our desired 9 ms when
      # cycle time doesn't allow for it. We already know the outputs have the
      # correct end time still
      relevent = results.filter_between(t1, t4)
      underdwelled = filter(lambda x: x.update.calculations.dwell_us < 8500, relevent.filter_updates())
      self.assertTrue(len(list(underdwelled)) > 0)

      # There should be no dwells below our minimum dwell, with a tad of wiggle
      # room
      baddwells = list(filter(
              lambda x: x.config.typ == OutputConfig.IGN and x.duration_us < 5900,
              relevent.filter_enriched_outputs()))
      self.assertTrue(len(baddwells) == 0)

      last_output = results.filter_enriched_outputs()[-1]
      remaining_feeds = results.filter_between(last_output.time,
                                               t4).filter_updates()
      self.assertTrue(len(remaining_feeds) > 0)
      for f in remaining_feeds:
          self.assertTrue(f.update.calculations.dwell_overduty_cut)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        sys.argv = sys.argv[1:] # Pass commandline options to test
    unittest.main(argv=sys.argv)
