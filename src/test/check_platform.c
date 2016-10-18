#include "scheduler.h"
#include "platform.h"
#include "decoder.h"
#include "check_platform.h"


static timeval_t curtime = 0;
static timeval_t cureventtime = 0;
static int int_on = 1;
static int output_state = 0;

void check_platform_reset() {
  curtime = 0;
}

void disable_interrupts() {
  int_on = 0;
}

void enable_interrupts() {
  int_on = 1;
}

timeval_t current_time() {
  return curtime;
}

void set_current_time(timeval_t t) {
  curtime = t;
}

void set_event_timer(timeval_t t) {
  cureventtime = t;
}

timeval_t get_event_timer() {
  return cureventtime;
}

void clear_event_timer() {
  cureventtime = 0;
}

void disable_event_timer() {
  cureventtime = 0;
}

int interrupts_enabled() {
  return int_on;
}

void set_output(int output, char value) {
  output_state = value;  
}

int get_output(int output) {
  return output_state;
}

void adc_gather(void *_adc) {
}

int usart_tx_ready() {
  return 0;
}

int usart_rx_ready() {
  return 0;
}

void usart_rx_reset() {
}

void usart_tx(char *s, unsigned short len) {
}

int current_output_buffer() {
  return 0;
}

timeval_t init_output_thread(uint32_t *b0, uint32_t *b1, uint32_t len) {
  return 0;
}


void enable_test_trigger(trigger_type trig, unsigned int rpm) {
}

void platform_save_config() {

}

void platform_load_config() {

}
