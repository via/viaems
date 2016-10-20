#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
#include "sensors.h"
#include "table.h"
#include "config.h"
#include "calculations.h"
#include "scheduler.h"
#include <check.h>
#include "queue.h"

#include "check_platform.h"


static int value_within(int percent, float value, float target) {
  float diff = (target - value) / target;
  return (diff * 100) <= percent;
}


START_TEST(check_rpm_from_time_diff) {
  /* 360 degrees for 0.005 s is 6000 rpm */
  ck_assert(value_within(1, rpm_from_time_diff(2000, 180), 6000));

  /* 90 degrees for 0.00125 s is 6000 rpm */
  ck_assert(value_within(1, rpm_from_time_diff(5000, 45), 6000));
} END_TEST

START_TEST(check_time_from_rpm_diff) {
  /* 6000 RPMS, 360 degrees = 0.005 s */
  ck_assert(value_within(1, time_from_rpm_diff(6000, 180), 5000));

  /* 6000 RPMS, 90 is 0.00125 s */
  ck_assert(value_within(1, time_from_rpm_diff(6000, 45), 1250));
} END_TEST

START_TEST(check_time_from_us) {
  ck_assert_int_eq(time_from_us(0), 0);
  ck_assert_int_eq(time_from_us(1000), 1000 * (TICKRATE / 1000000));
} END_TEST

START_TEST(check_time_in_range) {
  timeval_t t1, t2, val;

  t1 = 0xFF000000;
  val = 0xFF0000FF;
  t2 = 0xFFFF0000;
  ck_assert_int_eq(time_in_range(val, t1, t2), 1);

  t1 = 0xFF000000;
  val = 0x80000000;
  t2 = 0xFFFF0000;
  ck_assert_int_eq(time_in_range(val, t1, t2), 0);

  t1 = 0xFF000000;
  val = 0xFF0000FF;
  t2 = 0xFFFF0000;
  ck_assert_int_eq(time_in_range(val, t2, t1), 0);

  t1 = 0xFF000000;
  val = 0x0000FFFF;
  t2 = 0x40000000;
  ck_assert_int_eq(time_in_range(val, t1, t2), 1);

  t1 = 0x1000;
  val = 0x2000;
  t2 = 0x1000;
  ck_assert_int_eq(time_in_range(val, t1, t2), 0);

  t1 = 0x1000;
  val = 0x1000;
  t2 = 0x1000;
  ck_assert_int_eq(time_in_range(val, t1, t2), 1);
} END_TEST

START_TEST(check_time_diff) {
  timeval_t t1, t2;

  t1 = 0xFF000000;
  t2 = 0xFF0000FF;
  ck_assert_int_eq(time_diff(t2, t1), 0xFF);

  t1 = 0xFFFFFF00;
  t2 = 0x00000010;
  ck_assert_int_eq(time_diff(t2, t1), 0x110);

  t1 = 0xFFFFFF00;
  t2 = 0xFFFFFF00;
  ck_assert_int_eq(time_diff(t2, t1), 0);
} END_TEST

START_TEST(check_clamp_angle) {
  ck_assert_int_eq(clamp_angle(0, 720), 0);
  ck_assert_int_eq(clamp_angle(-360, 720), 360);
  ck_assert_int_eq(clamp_angle(-1080, 720), 360);
  ck_assert_int_eq(clamp_angle(720, 720), 0);
  ck_assert_int_eq(clamp_angle(1080, 720), 360);
} END_TEST

START_TEST(check_sensor_process_linear) {
  struct sensor_input si = {
    .params = {
      .range = { .min=-10.0, .max=10.0},
    },
  };

  si.raw_value = 0;
  sensor_process_linear(&si);  
  ck_assert(si.processed_value == -10.0);

  si.raw_value = 4096.0;
  sensor_process_linear(&si);  
  ck_assert(si.processed_value == 10.0);

  si.raw_value = 2048.0;
  sensor_process_linear(&si);  
  ck_assert(si.processed_value == 0.0);

} END_TEST

START_TEST(check_sensor_process_freq) {
  struct sensor_input si;

  si.raw_value = 100.0;
  sensor_process_freq(&si);  
  ck_assert(value_within(1, si.processed_value, 9.765625));

  si.raw_value = 1000.0;
  sensor_process_freq(&si); 
  ck_assert(value_within(1, si.processed_value, 0.976562));

  si.raw_value = 0.0;
  sensor_process_freq(&si);  
  ck_assert(si.processed_value == 0.0);

} END_TEST

struct table t1 = {
  .num_axis = 1,
  .axis = { { 
    .num = 4,
    .values = {5, 10, 15, 20},
   } },
   .data = {
     .one = {50, 100, 150, 200}
   },
};

struct table t2 = {
  .num_axis = 2,
  .axis = { { 
    .num = 4,
    .values = {5, 10, 15, 20},
   }, {
    .num = 4,
    .values = {-50, -40, -30, -20}
    } },
   .data = {
     .two = { {50, 100, 250, 200},
              {60, 110, 260, 210},
              {70, 120, 270, 220},
              {80, 130, 280, 230} },
   },
};

START_TEST(check_table_oneaxis_interpolate) {
  ck_assert(interpolate_table_oneaxis(&t1, 7.5) == 75);
  ck_assert(interpolate_table_oneaxis(&t1, 5) == 50);
  ck_assert(interpolate_table_oneaxis(&t1, 10) == 100);
  ck_assert(interpolate_table_oneaxis(&t1, 20) == 200);
} END_TEST

START_TEST(check_table_oneaxis_clamp) {
  ck_assert(interpolate_table_oneaxis(&t1, 0) == 50);
  ck_assert(interpolate_table_oneaxis(&t1, 30) == 200);
} END_TEST

START_TEST(check_table_twoaxis_interpolate) {
  ck_assert(interpolate_table_twoaxis(&t2, 5, -50) == 50);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -50) == 75);
  ck_assert(interpolate_table_twoaxis(&t2, 5, -45) == 55);
  ck_assert(interpolate_table_twoaxis(&t2, 7.5, -45) == 80);
} END_TEST

START_TEST(check_table_twoaxis_clamp) {
  ck_assert(interpolate_table_twoaxis(&t2, 10, -60) == 100);
  ck_assert(interpolate_table_twoaxis(&t2, 10, 0) == 130);
  ck_assert(interpolate_table_twoaxis(&t2, 0, -40) == 60);
  ck_assert(interpolate_table_twoaxis(&t2, 30, -40) == 210);
  ck_assert(interpolate_table_twoaxis(&t2, 30, -45) == 205);
} END_TEST

START_TEST(check_calculate_ignition_cut) {
  config.rpm_stop = 5000;
  config.rpm_start = 4500;

  config.decoder.rpm = 1000;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 5500;
  ck_assert_int_eq(ignition_cut(), 1);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 1);

  config.decoder.rpm = 4000;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 4800;
  ck_assert_int_eq(ignition_cut(), 0);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 0);

  config.decoder.rpm = 5500;
  ck_assert_int_eq(ignition_cut(), 1);
  ck_assert_int_eq(calculated_values.rpm_limit_cut, 1);
} END_TEST

START_TEST(check_calculate_ignition_fixedduty) {
  struct table t = { 
    .num_axis = 2,
    .axis = { { .num = 2, .values = {5, 10} }, { .num = 2, .values = {5, 10}} },
    .data = { .two = { {10, 10}, {10, 10} } },
  };
  config.timing = &t;
  config.ignition.dwell = DWELL_FIXED_DUTY;
  config.decoder.rpm = 6000;

  calculate_ignition();
  
  ck_assert(calculated_values.timing_advance == 10);
  /* 10 ms per rev, dwell should be 1/8 of rotation,
   * fuzzy estimate because math */
  ck_assert(abs(calculated_values.dwell_us - (10000 / 8)) < 5);
} END_TEST
    

int main(void) {

  Suite *tfi_suite = suite_create("TFI");
  TCase *util_tests = tcase_create("util");
  TCase *sensor_tests = tcase_create("sensors");
  TCase *table_tests = tcase_create("tables");
  TCase *calc_tests = tcase_create("calculations");


  tcase_add_test(util_tests, check_rpm_from_time_diff);
  tcase_add_test(util_tests, check_time_from_rpm_diff);
  tcase_add_test(util_tests, check_time_in_range);
  tcase_add_test(util_tests, check_time_diff);
  tcase_add_test(util_tests, check_clamp_angle);
  tcase_add_test(util_tests, check_time_from_us);

  tcase_add_test(sensor_tests, check_sensor_process_linear);
  tcase_add_test(sensor_tests, check_sensor_process_freq);

  tcase_add_test(table_tests, check_table_oneaxis_interpolate);
  tcase_add_test(table_tests, check_table_oneaxis_clamp);
  tcase_add_test(table_tests, check_table_twoaxis_interpolate);
  tcase_add_test(table_tests, check_table_twoaxis_clamp);

  tcase_add_test(calc_tests, check_calculate_ignition_cut);
  tcase_add_test(calc_tests, check_calculate_ignition_fixedduty);

  suite_add_tcase(tfi_suite, util_tests);
  suite_add_tcase(tfi_suite, sensor_tests);
  suite_add_tcase(tfi_suite, table_tests);
  suite_add_tcase(tfi_suite, calc_tests);
  suite_add_tcase(tfi_suite, setup_decoder_tests());
  suite_add_tcase(tfi_suite, setup_scheduler_tests());
  SRunner *sr = srunner_create(tfi_suite);
  srunner_run_all(sr, CK_VERBOSE);
  return 0;
}

