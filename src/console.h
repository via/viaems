#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <cbor.h>

#include "platform.h"

struct console_feed_update {
  timeval_t time;

  float ve;
  float lambda;
  uint32_t fuel_pulsewidth_us;
  float temp_enrich_percent;
  float injector_dead_time;
  float injector_pw_correction;
  float accel_enrich_percent;
  float airmass_per_cycle;

  float advance;
  uint32_t dwell_us;
  bool rpm_cut;
  bool boost_cut;
  bool fuel_overduty_cut;
  bool dwell_overduty_cut;

  float map;
  float iat;
  float clt;
  float brv;
  float tps;
  float tpsrate;
  float aap;
  float frt;
  float ego;
  float frp;
  float knock1;
  float knock2;
  float eth;
  uint32_t sensor_faults;

  uint32_t rpm;
  uint32_t tooth_rpm;
  bool sync;
  uint32_t loss_reason;
  float rpm_variance;
  float last_angle;
  uint32_t t0_count;
  uint32_t t1_count;
};

struct logged_event {
  timeval_t time;
  uint32_t seq;
  enum {
    EVENT_NONE,
    EVENT_OUTPUT,
    EVENT_GPIO,
    EVENT_TRIGGER,
  } type;
  uint32_t value;
};

struct config;
void console_process(struct config *config, timeval_t now);
void console_record_event(struct logged_event);
void record_engine_update(const struct viaems *viaems,
                          const struct engine_update *eng_update,
                          const struct calculated_values *calcs);

#ifdef UNITTEST
#include <check.h>
TCase *setup_console_tests(void);
#endif

#endif
