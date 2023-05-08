#ifndef _COMMON_TLV2553_ADC_H
#define _COMMON_TLV2553_ADC_H

#include <stdint.h>

#define NUM_SPI_TX 30
#define SPI_INPUT(X) ((X << 12) | (0x0C00)) /* Specify 16 bit frames */

/* TLV2553 can be driven up to 15 MHz. The best we can do with the STM32F4
is 10.4 MHz, though we can do 15 MHz with the GD32F470.  However, even at 15 we
can't do 200 ksps with 16 bit transfers (only 12), which is not an option on
either chip, since there's up to 4.15 uS Tconv time.  On all platforms lets run
at 150 ksps, which gives us plenty of room even at 7.5 MHz for a falling/rising
CS between samples (CS can rise immediately after data is sent, it doesn't need
to be after Tconv has passed).

We want two high frequency knock inputs, so for a sampling strategy we sample
those two inputs each time we sample one of the other inputs. There are 11
inputs, and 3 self-test inputs. We'll use one of vref/2 self test input for a
total of 12 low speed and 2 high speed inputs. At 150 ksps, this gives us 5 KHz
for the low speed inputs, and 50 KHz for the knock inputs */

/* clang-format off */
static const uint16_t adc_transmit_sequence[NUM_SPI_TX] = {
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(2),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(3),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(4),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(5),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(6),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(7),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(8),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(9),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(10),
  SPI_INPUT(0), SPI_INPUT(1),  SPI_INPUT(11),
};
/* clang-format on */

/* The 12 bit value from the ADC is in a 16 bit frame, MSB first, with four
 * trailing zeros */
static inline uint16_t read_raw_from_position(const uint16_t *values,
                                              int position) {
  return values[position] >> 4;
}

/* The result from any of the commands is in the following SPI response, so all
 * response indexes are +1 from the transmit command list.  The self-test
 * response therefore is always the first value. */
static inline uint16_t read_adc_pin(const uint16_t *values, int pin) {
  switch (pin) {
  case 2:
    return read_raw_from_position(values, 3);
  case 3:
    return read_raw_from_position(values, 6);
  case 4:
    return read_raw_from_position(values, 9);
  case 5:
    return read_raw_from_position(values, 12);
  case 6:
    return read_raw_from_position(values, 15);
  case 7:
    return read_raw_from_position(values, 18);
  case 8:
    return read_raw_from_position(values, 21);
  case 9:
    return read_raw_from_position(values, 24);
  case 10:
    return read_raw_from_position(values, 27);
  case 11:
    /* This self-test result will be from the previous transmit set */
    return read_raw_from_position(values, 0);
  default:
    return 0;
  }
}

/* Self test pin is (Vref+ + Vref-) / 2, which should be 2048 */
static inline bool adc_response_is_valid(const uint16_t *values) {
  uint16_t self_test = read_adc_pin(values, 11);
  if ((self_test < (2048 - 10)) || (self_test > (2048 + 10))) {
    return false;
  }
  return true;
}

static void process_knock_inputs(const uint16_t *values) {
  for (int i = 0; i < 10; i++) {
    float in1 = (float)read_raw_from_position(values, (i * 3) + 1) / 4095.0f;
    knock_add_sample(&config.knock_inputs[0], in1);

    float in2 = (float)read_raw_from_position(values, (i * 3) + 2) / 4095.0f;
    knock_add_sample(&config.knock_inputs[1], in2);
  }
}

#endif
