#include "platform.h"
#include "config.h"
#include "decoder.h"
#include "check_platform.h"

#include <check.h>

struct decoder_event {
  unsigned int t0 : 1;
  unsigned int t1 : 1;
  timeval_t time;
  decoder_state state;
  int valid;
  decoder_loss_reason reason;
}; 


static struct decoder_event tfi_startup_events[] = {
  {1, 0, 18000, DECODER_NOSYNC, 0, 0},
  {1, 0, 25000, DECODER_NOSYNC, 0, 0},
  {1, 0, 50000, DECODER_NOSYNC, 0, 0},
  {1, 0, 75000, DECODER_SYNC, 1, 0},
  {1, 0, 100000, DECODER_SYNC, 1, 0},
  {1, 0, 125000, DECODER_SYNC, 1, 0},
};

void validate_decoder_sequence(struct decoder_event *ev, int num) {
  int i;
  int correct_rpm;
  timeval_t correct_expire;
  for (i = 0; i < num; ++i) {
    if (ev[i].t0) {
      config.decoder.last_t0 = ev[i].time;
      config.decoder.needs_decoding_t0 = 1;
    }
    if (ev[i].t1) {
      config.decoder.last_t1 = ev[i].time;
      config.decoder.needs_decoding_t1 = 1;
    }

    set_current_time(ev[i].time);
    config.decoder.decode(&config.decoder);
    ck_assert_msg(config.decoder.state == ev[i].state, 
        "expected event at %d: (%d, %d). Got (%d, %d)",
        ev[i].time, ev[i].state, ev[i].valid, 
        config.decoder.state, config.decoder.valid);
    if (!ev[i].valid) {
      ck_assert_msg(config.decoder.loss == ev[i].reason,
      "reason mismatch at %d: %d. Got %d",
      ev[i].time, ev[i].reason, config.decoder.loss);
    }
  }
}

static void prepare_decoder(trigger_type type) {
  config.trigger = type;
  decoder_init(&config.decoder);
}


START_TEST(check_tfi_decoder_startup_normal) {

  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);
  ck_assert_int_eq(config.decoder.last_trigger_angle, 180);

} END_TEST

START_TEST(check_tfi_decoder_syncloss_variation) {

  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);

  struct decoder_event ev[] = {
    {1, 0, 150000, DECODER_SYNC, 1, 0},
    {1, 0, 155000, DECODER_NOSYNC, 0, DECODER_VARIATION},
  };
  validate_decoder_sequence(ev, 2);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
} END_TEST

START_TEST(check_tfi_decoder_syncloss_expire) {
  prepare_decoder(FORD_TFI);
  validate_decoder_sequence(tfi_startup_events, 6);
  ck_assert_int_eq(config.decoder.expiration, 162500);

  set_current_time(170000);
  check_handle_decoder_expire(&config.decoder);
  ck_assert_int_eq(0, config.decoder.current_triggers_rpm);
  ck_assert_int_eq(0, config.decoder.valid);
  ck_assert_int_eq(DECODER_EXPIRED, config.decoder.loss);
} END_TEST

TCase *setup_decoder_tests() {
  TCase *decoder_tests = tcase_create("decoder");
  tcase_add_test(decoder_tests, check_tfi_decoder_startup_normal);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_variation);
  tcase_add_test(decoder_tests, check_tfi_decoder_syncloss_expire);
  return decoder_tests;
}
