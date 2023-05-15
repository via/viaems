#ifndef _COMMON_AD7888_ADC_H
#define _COMMON_AD7888_ADC_H

#include <stdint.h>

#define NUM_SPI_TX 9
#define SPI_INPUT(X) ((X << 11) | 0x400) /* External reference */

/* AD7888 can sample up to 125 ksps with a SPI bus of 2 MHz. There is no self
 * test input.  We just collect all 8 inputs, plus one to read back the last
 * result */
static const uint16_t adc_transmit_sequence[NUM_SPI_TX] = {
  SPI_INPUT(0), SPI_INPUT(1), SPI_INPUT(2), SPI_INPUT(3), SPI_INPUT(4),
  SPI_INPUT(5), SPI_INPUT(6), SPI_INPUT(7), SPI_INPUT(0),
};

/* The 12 bit value from the ADC is in a 16 bit frame, MSB first, with four
 * leading zeros */
static inline uint16_t read_raw_from_position(const uint16_t *values,
                                              int position) {
  return values[position] & 0xFFF;
}

/* The result from any of the commands is in the following SPI response, so all
 * response indexes are +1 from the transmit command list. */
static inline uint16_t read_adc_pin(const uint16_t *values, int pin) {
  switch (pin) {
  case 0:
    return read_raw_from_position(values, 1);
  case 1:
    return read_raw_from_position(values, 2);
  case 2:
    return read_raw_from_position(values, 3);
  case 3:
    return read_raw_from_position(values, 4);
  case 4:
    return read_raw_from_position(values, 5);
  case 5:
    return read_raw_from_position(values, 6);
  case 6:
    return read_raw_from_position(values, 7);
  case 7:
    return read_raw_from_position(values, 8);
  default:
    return 0;
  }
}

/* No self test exists */
static inline bool adc_response_is_valid(const uint16_t *values) {
  (void)values;
  return true;
}

#endif
