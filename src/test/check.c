#include <stdio.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
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
  ck_assert(value_within(1, rpm_from_time_diff(500000, 180), 6000));

  /* 90 degrees for 0.00125 s is 6000 rpm */
  ck_assert(value_within(1, rpm_from_time_diff(125000, 45), 6000));
} END_TEST

START_TEST(check_time_from_rpm_diff) {
  /* 6000 RPMS, 360 degrees = 0.005 s */
  ck_assert(value_within(1, time_from_rpm_diff(6000, 180), 5000));

  /* 6000 RPMS, 90 is 0.00125 s */
  ck_assert(value_within(1, time_from_rpm_diff(6000, 45), 1250));
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


int main(void) {

  Suite *tfi_suite = suite_create("TFI");
  TCase *util_tests = tcase_create("util");


  tcase_add_test(util_tests, check_rpm_from_time_diff);
  tcase_add_test(util_tests, check_time_from_rpm_diff);
  tcase_add_test(util_tests, check_time_in_range);
  tcase_add_test(util_tests, check_time_diff);
  tcase_add_test(util_tests, check_clamp_angle);

  suite_add_tcase(tfi_suite, util_tests);
  suite_add_tcase(tfi_suite, setup_decoder_tests());
  suite_add_tcase(tfi_suite, setup_scheduler_tests());
  SRunner *sr = srunner_create(tfi_suite);
  srunner_run_all(sr, CK_VERBOSE);
  return 0;
}

