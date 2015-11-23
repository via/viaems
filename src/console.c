#include "platform.h"
#include "console.h"
#include "config.h"
#include "calculations.h"

#include <stdio.h>
#include <string.h>

void console_process() {

  /*  if tx buffer is usable, print regular status update */
  if (!usart_tx_ready()) {
    return;
  }


  snprintf(config.console.txbuffer, CONSOLE_BUFFER_SIZE,
      "* rpm=%d sync=%d t0_count=%d map=%f adv=%f\r\n",
      config.decoder.rpm,
      config.decoder.valid,
      config.decoder.t0_count,
      config.adc[ADC_MAP].processed_value,
      calculated_values.timing_advance);

  usart_tx(config.console.txbuffer, strlen(config.console.txbuffer));

  /* if rx buffer is available, process it */

}
