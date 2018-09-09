#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "sensors.h"
#include "platform.h"
#include "scheduler.h"
#include "hostcmd.h"

static struct timed_callback hostcmd_cb = {0};
static int control_socket;

/* Set initial callback */
void hostcmd_init() {
  hostcmd_cb.callback = hostcmd_callback;
  /* Stop us 1 second into running */
  schedule_callback(&hostcmd_cb, current_time() + TICKRATE);
}

/* Called when we've run as long as commanded to do so */
void hostcmd_callback() {
  int new_sensors = 0;

  /* Freeze the timers */
  platform_freeze_timers();
  timeval_t curtime = current_time();
  
  /* First send to host that we've stopped, and what the time at doing so is */
  printf("%lu STOPPED\n", curtime);

  /* Wait for a command, parse it*/
  char cmd[64];
  while (1) {
    if (!fgets(cmd, 64, stdin)) {
      printf("failed\n");
      return;
    }
    printf("got cmd = %s\n", cmd);

    if (!strncmp(cmd, "RUN", 3)) {
      timeval_t run_until_time = atoi(&cmd[4]);
      printf("run_until: %d\n", run_until_time);
      /* Schedule us to stop again, and break out */
      schedule_callback(&hostcmd_cb, run_until_time);
      break;
    }
    if (!strncmp(cmd, "TRIGGER1", 8)) {
      config.decoder.last_t0 = curtime;
      config.decoder.needs_decoding_t0 = 1;
    }
    if (!strncmp(cmd, "ADC", 3)) {
      int sensor, value;
      sscanf(&cmd[4], "%d %d", &sensor, &value);
      config.sensors[sensor].raw_value = value & 0xFFF;
      config.sensors[sensor].fault = FAULT_NONE;
      if (config.sensors[sensor].source == SENSOR_FREQ) {
        sensor_freq_new_data();
        new_sensors = 1;
      }
      if (config.sensors[sensor].source == SENSOR_ADC) {
        sensor_adc_new_data();
        new_sensors = 1;
      }
    }
  }

  /* Re-start timers */
  platform_unfreeze_timers();

  /* While timers are running again, handle any processing we need */
  if (new_sensors) {
    sensors_process();
  }
  if (config.decoder.needs_decoding_t0 || config.decoder.needs_decoding_t1) {
    decoder_update_scheduling();
  }
}

