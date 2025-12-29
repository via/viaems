#include <assert.h>

#include "limits.h"
#include "platform.h"
#include "util.h"

const float tick_degree_rpm_ratio = (TICKRATE / 6.0f);

unsigned int rpm_from_time_diff(timeval_t t1, degrees_t deg) {
  float ticks_per_degree = t1 / deg;
  unsigned int rpm = tick_degree_rpm_ratio / ticks_per_degree;
  return rpm;
}

timeval_t time_from_rpm_diff(unsigned int rpm, degrees_t deg) {
  float ticks_per_degree = tick_degree_rpm_ratio / (float)rpm;
  return ticks_per_degree * deg;
}

degrees_t degrees_from_time_diff(timeval_t t, unsigned int rpm) {
  float ticks_per_degree = tick_degree_rpm_ratio / (float)rpm;
  return t / ticks_per_degree;
}

timeval_t time_from_us(unsigned int us) {
  timeval_t ticks = us * (TICKRATE / 1000000.0f);
  return ticks;
}

unsigned int us_from_time(timeval_t time) {
  unsigned int us = time / (TICKRATE / 1000000.0f);
  return us;
}

/* True if n is before x */
bool time_before(timeval_t n, timeval_t x) {
  signed int res = n - x;
  return (res < 0);
}

bool time_before_or_equal(timeval_t n, timeval_t x) {
  signed int res = n - x;
  return (res <= 0);
}

bool time_in_range(timeval_t val, timeval_t t1, timeval_t t2) {
  return (val - t1) <= (t2 - t1);
}

timeval_t time_diff(timeval_t later, timeval_t earlier) {
  return later - earlier;
}

/* Attempt to normalize an angle to [0, max) degrees.
 * Assumes an angle within one full cycle of the range [-max, 2*max).
 */
degrees_t clamp_angle(const degrees_t ang, const degrees_t max) {
  assert(ang >= -max);
  assert(ang < max * 2);

  float result = ang;

  if (ang < 0) {
    result += max;
  } else if (ang >= max) {
    result -= max;
  }
  return result;
}

#ifdef UNITTEST
#include <check.h>

START_TEST(check_rpm_from_time_diff) {
  /* 180 degrees for 0.005 s is 6000 rpm */
  ck_assert_float_eq_tol(rpm_from_time_diff(20000, 180), 6000, 50);

  /* 90 degrees for 0.00125 s is 6000 rpm */
  ck_assert_float_eq_tol(rpm_from_time_diff(5000, 45), 6000, 50);
}
END_TEST

START_TEST(check_time_from_rpm_diff) {
  /* 6000 RPMS, 180 degrees = 0.005 s */
  ck_assert_float_eq_tol(time_from_rpm_diff(6000, 180), 20000, 50);

  /* 6000 RPMS, 90 is 0.00125 s */
  ck_assert_float_eq_tol(time_from_rpm_diff(6000, 45), 5000, 50);
}
END_TEST

START_TEST(check_degrees_from_time_diff) {
  /* 6000 RPMS, 180 degrees = 0.005 s */
  ck_assert_float_eq_tol(degrees_from_time_diff(20000, 6000), 180, 1);

  /* 6000 RPMS, 90 is 0.00125 s */
  ck_assert_float_eq_tol(degrees_from_time_diff(5000, 6000), 45, 1);
}
END_TEST

START_TEST(check_time_from_us) {
  ck_assert_int_eq(time_from_us(0), 0);
  ck_assert_int_eq(time_from_us(1000), 1000 * (TICKRATE / 1000000));
}
END_TEST

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
}
END_TEST

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
}
END_TEST

START_TEST(check_clamp_angle) {
  ck_assert_int_eq(clamp_angle(0, 720), 0);
  ck_assert_int_eq(clamp_angle(-360, 720), 360);
  ck_assert_int_eq(clamp_angle(720, 720), 0);
  ck_assert_int_eq(clamp_angle(1080, 720), 360);
}
END_TEST

TCase *setup_util_tests() {
  TCase *util_tests = tcase_create("util");
  tcase_add_test(util_tests, check_rpm_from_time_diff);
  tcase_add_test(util_tests, check_time_from_rpm_diff);
  tcase_add_test(util_tests, check_degrees_from_time_diff);
  tcase_add_test(util_tests, check_time_in_range);
  tcase_add_test(util_tests, check_time_diff);
  tcase_add_test(util_tests, check_clamp_angle);
  tcase_add_test(util_tests, check_time_from_us);
  return util_tests;
}

#endif
