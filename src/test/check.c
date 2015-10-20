#include <stdio.h>
#include "platform.h"
#include "util.h"
#include "decoder.h"
#include <check.h>

struct decoder_event {
  timeval_t time;
  int valid;
  int rpm;
}; 

void tfi_pip_decoder(struct decoder *);

void validate_decoder_sequence(struct decoder *d, struct decoder_event *ev, 
                               int num) {
  int i;
  for (i = 0; i < num; ++i) {
    d->last_t0 = ev[i].time;
    tfi_pip_decoder(d);
    ck_assert_msg(d->valid == ev[i].valid, "expected: {%d, %d, %d} got: {%d, %d, %d}",
        ev[i].time, ev[i].valid, ev[i].rpm, d->last_trigger_time, d->valid, d->rpm);
    if (ev[i].valid) {
      ck_assert_int_eq(d->rpm, ev[i].rpm);
    }
  }
}

void disable_interrupts() {
}

void enable_interrupts() {
}

timeval_t current_time() {
  return 0;
}

START_TEST(check_decoder_startup_rough) {
} END_TEST

START_TEST(check_decoder_startup_normal) {
  struct decoder d1;
  struct decoder_event ev1[] = {
    {18000, 0, 0},
    {25000, 0, 0},
    {50000, 0, 0},
    {75000, 0, 0},
    {100000, 1, 6000},
    {125000, 1, 6000},
  };

  validate_decoder_sequence(&d1, ev1, 6);

  struct decoder d2;
  struct decoder_event ev2[] = {
    {0xFFFD6000, 0, 0},
    {0xFFFDE000, 0, 0},
    {0xFFFE6000, 0, 0},
    {0xFFFEE000, 1, 4577},
    {0xFFFF6000, 1, 4577},
    {0xFFFFE000, 1, 4577},
    {0x00006000, 1, 4577},
    {0x0000E000, 1, 4577},
  };

  validate_decoder_sequence(&d2, ev2, 8);

} END_TEST

START_TEST(check_decoder_normal_running) {
  struct decoder d;
  struct decoder_event ev[] = {
    {0, 0, 0},
    {25000, 0, 0},
    {50000, 0, 0},
    {75000, 1, 6000},
    {100000, 1, 6000},
    {125000, 1, 6000},
    {150000, 1, 6000},
    {175000, 1, 6000},
    {200000, 1, 6000},
    {225000, 1, 6000},
  };

  validate_decoder_sequence(&d, ev, 10);
} END_TEST

START_TEST(check_decoder_normal_running_weird_clocks) {
  struct decoder d;
  struct decoder_event ev[] = {
    {0, 0, 0},
    {20000, 0, 0},
    {51500, 0, 0},
    {76200, 1, 5905},
    {103000, 1, 5421},
    {128240, 1, 5863},
    {153720, 1, 5804},
    {178370, 1, 5970},
    {202850, 1, 6031},
    {227440, 1, 6104},
  };
  validate_decoder_sequence(&d, ev, 10);
} END_TEST

START_TEST(check_decoder_normal_regain_sync) {
  struct decoder d;
  struct decoder_event ev[] = {
    {0, 0, 0},
    {20000, 0, 0},
    {51500, 0, 0},
    {76200, 1, 5905},
    {103000, 1, 5421},
    {130240, 0, 5863},
    {153720, 0, 5804},
    {178370, 1, 5970},
    {202850, 1, 6197},
    {227440, 1, 6104},
  };
  validate_decoder_sequence(&d, ev, 10);
} END_TEST

START_TEST(check_rpm_from_time_diff) {
  /* 180 degrees for 0.005 s is 6000 rpm */
  ck_assert_int_eq(rpm_from_time_diff(100000, 180), 6000);

  /* 45 degrees for 0.00125 s is 6000 rpm */
  ck_assert_int_eq(rpm_from_time_diff(25000, 45), 6000);
} END_TEST

START_TEST(check_time_from_rpm_diff) {
  /* 6000 RPMS, 180 degrees = 0.005 s */
  ck_assert_int_eq(time_from_rpm_diff(6000, 180), 100000);

  /* 6000 RPMS, 45 is 0.00125 s */
  ck_assert_int_eq(time_from_rpm_diff(6000, 45), 25000);
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

int main(void) {

  Suite *tfi_suite = suite_create("TFI");
  TCase *decoder_tests = tcase_create("tfi_pip_decoder");
  TCase *util_tests = tcase_create("util");

  tcase_add_test(decoder_tests, check_decoder_startup_rough);
  tcase_add_test(decoder_tests, check_decoder_startup_normal);
  tcase_add_test(decoder_tests, check_decoder_normal_running);
  tcase_add_test(decoder_tests, check_decoder_normal_running_weird_clocks);
  tcase_add_test(decoder_tests, check_decoder_normal_regain_sync);

  tcase_add_test(util_tests, check_rpm_from_time_diff);
  tcase_add_test(util_tests, check_time_from_rpm_diff);
  tcase_add_test(util_tests, check_time_in_range);
  tcase_add_test(util_tests, check_time_diff);

  suite_add_tcase(tfi_suite, decoder_tests);
  suite_add_tcase(tfi_suite, util_tests);
  SRunner *sr = srunner_create(tfi_suite);
  srunner_run_all(sr, CK_NORMAL);
  return 0;
}

