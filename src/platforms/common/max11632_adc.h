#ifndef _COMMON_MAX11632_ADC_H
#define _COMMON_MAX11632_ADC_H

#include <stdint.h>

#define NUM_SPI_TX 24
#define SPI_SETUP 0x7400 /* Clock mode 11, external reference */
#define SPI_INPUT(X) (0x8600 | ((X) << 11)) /* No scan */

/* Query the 16 inputs in a pattern that causes input 0 and 1 to get sampled at
 * 4x the rate of the rest, over a total of 24 spi commands.  The first command
 * sets the setup register. The order allows the last command to be a NOP, and
 * thus sample results don't cross buffer boundaries
 * */
static const uint16_t adc_transmit_sequence[NUM_SPI_TX] = {
  SPI_SETUP,     SPI_INPUT(2), SPI_INPUT(3),  SPI_INPUT(0),  SPI_INPUT(1),
  SPI_INPUT(4),  SPI_INPUT(5), SPI_INPUT(6),  SPI_INPUT(7),  SPI_INPUT(0),
  SPI_INPUT(1),  SPI_INPUT(8), SPI_INPUT(9),  SPI_INPUT(10), SPI_INPUT(11),
  SPI_INPUT(0),  SPI_INPUT(1), SPI_INPUT(12), SPI_INPUT(13), SPI_INPUT(14),
  SPI_INPUT(15), SPI_INPUT(0), SPI_INPUT(1),  0x1000,
};

static inline uint16_t read_raw_from_position(const uint16_t *values,
                                              int position) {
  uint16_t first = values[position];
  uint16_t second = values[position + 1];
  return ((first & 0x0F) << 8) | (second >> 8);
}

static inline uint16_t read_adc_pin(const uint16_t *values, int pin) {
  switch (pin) {
  case 2:
    return read_raw_from_position(values, 1);
  case 3:
    return read_raw_from_position(values, 2);
  case 4:
    return read_raw_from_position(values, 5);
  case 5:
    return read_raw_from_position(values, 6);
  case 6:
    return read_raw_from_position(values, 7);
  case 7:
    return read_raw_from_position(values, 8);
  case 8:
    return read_raw_from_position(values, 11);
  case 9:
    return read_raw_from_position(values, 12);
  case 10:
    return read_raw_from_position(values, 13);
  case 11:
    return read_raw_from_position(values, 14);
  case 12:
    return read_raw_from_position(values, 17);
  case 13:
    return read_raw_from_position(values, 18);
  case 14:
    return read_raw_from_position(values, 19);
  case 15:
    return read_raw_from_position(values, 20);
  default:
    return 0;
  }
}

/* MAX11632 has no self check inputs */
bool adc_response_is_valid(const uint16_t *values) {
  (void)values;
  return true;
}


#endif
