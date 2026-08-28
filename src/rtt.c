#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <string.h>

struct rtt_tx_buffer {
  const char *name;
  uint8_t *buffer;

  uint32_t buffer_size;
  uint32_t write_idx;
  volatile uint32_t read_idx;
  uint32_t flags;
};

struct rtt_rx_buffer {
  const char *name;
  uint8_t *buffer;

  uint32_t buffer_size;
  volatile uint32_t write_idx;
  uint32_t read_idx;
  uint32_t flags;
};

struct rtt_cb {
  char id[16];
  int32_t n_tx_buffers;
  int32_t n_rx_buffers;

  struct rtt_tx_buffer tx[1];
  struct rtt_rx_buffer rx[1];
};

__attribute__((aligned(32))) static uint8_t tx0_buffer_data[1024];
__attribute__((aligned(32))) static uint8_t rx0_buffer_data[64];

__attribute__((aligned(32)))
struct rtt_cb RTT = {
  .id = "SEGGER RTT",
  .n_tx_buffers = 1,
  .n_rx_buffers = 1,

  .tx = { {
    .name = "Debug TX",
    .buffer = tx0_buffer_data,
    .buffer_size = sizeof(tx0_buffer_data),
    .flags = 1,
  } },
  .rx = { {
    .name = "Debug TX",
    .buffer = rx0_buffer_data,
    .buffer_size = sizeof(rx0_buffer_data),
    .flags = 1,
  } },
};

static bool rtt_tx_full(void) {
  uint32_t next_write = RTT.tx[0].write_idx + 1;
  if (next_write >= RTT.tx[0].buffer_size) {
    next_write = 0;
  }

  if (next_write == RTT.tx[0].read_idx) {
    return true;
  } else {
    return false;
  }
}

static size_t rtt_tx_available(void) {

  uint32_t r = RTT.tx[0].read_idx;
  uint32_t w = RTT.tx[0].write_idx;

  if (r > w) {
    return (r - w) - 1;
  } else {
    uint32_t bufleft = RTT.tx[0].buffer_size - w;
    return (bufleft + r) - 1;
  }
}

bool rtt_write(const uint8_t *data, const size_t len) {
  if (len > rtt_tx_available()) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    RTT.tx[0].buffer[RTT.tx[0].write_idx] = data[i];
    uint32_t next_write = RTT.tx[0].write_idx + 1;
    if (next_write >= RTT.tx[0].buffer_size) {
      next_write = 0;
    }
    RTT.tx[0].write_idx = next_write;
  }
  return true;
}

char nibble_to_char(uint8_t b) {
  b &= 0x0F;
  if (b < 10) {
    return '0' + b;
  } else {
    return 'A' + (b - 10);
  }
}


void itoa(char dst[8], uint32_t x) {
  for (int i = 7; i >= 0; i--) {
    dst[i] = nibble_to_char(x);
    x >>= 4;
  }
}

void rtt_debug(const char *data, uint32_t W, uint32_t X, uint32_t Y, uint32_t Z) {

  char line[128];
  size_t data_len = strlen(data);

  if (data_len > 64) {
    data_len = 64;
  }

  memcpy(line, data, data_len);

  line[data_len] = ':';
  line[data_len + 1] = ' ';

  itoa(&line[data_len + 2], W);
  line[data_len + 2 + 8] = ' ';

  itoa(&line[data_len + 2 + 9], X);
  line[data_len + 2 + 8 + 9] = ' ';
  
  itoa(&line[data_len + 2 + 18], Y);
  line[data_len + 2 + 8 + 18] = ' ';

  itoa(&line[data_len + 2 + 27], Z);
  line[data_len + 2 + 8 + 27] = '\n';

  rtt_write(line, data_len + 2 + 8 + 27 + 1);

}
