#include <stdio.h>
#include "platform.h"
#include <check.h>

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
} END_TEST

START_TEST(check_decoder_normal_running) {
} END_TEST

START_TEST(check_decoder_normal_running_weird_clocks) {
} END_TEST

START_TEST(check_decoder_normal_too_much_variation) {
} END_TEST

START_TEST(check_decoder_normal_regain_sync) {
} END_TEST

START_TEST(check_rpm_from_time_diff) {
} END_TEST

START_TEST(check_time_from_rpm_diff) {
} END_TEST

START_TEST(check_time_in_range) {
} END_TEST

START_TEST(check_time_diff) {
} END_TEST

int main(void) {

  Suite *tfi_suite = suite_create("TFI");
  TCase *decoder_tests = tcase_create("tfi_pip_decoder");
  TCase *util_tests = tcase_create("util");

  tcase_add_test(decoder_tests, check_decoder_startup_rough);
  tcase_add_test(decoder_tests, check_decoder_startup_normal);
  tcase_add_test(decoder_tests, check_decoder_normal_running);
  tcase_add_test(decoder_tests, check_decoder_normal_running_weird_clocks);
  tcase_add_test(decoder_tests, check_decoder_normal_too_much_variation);
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

