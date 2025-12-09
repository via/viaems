#ifndef S32K148_ENET_H
#define S32K148_ENET_H

#include <stdint.h>
#include <stdbool.h>

struct enet_config {
  uint8_t ethernet_address[6];
  uint32_t ip_address;
  uint16_t udp_port;

  uint32_t dest_mcast_addr;
};


void enet_initialize(const struct enet_config *config);

struct enet_tx_descriptor;
bool enet_get_next_available_frame_tx(struct enet_tx_descriptor **desc);
void enet_tx_frame_set_dest(struct enet_tx_descriptor *desc, uint8_t address[6]);
void enet_tx_frame_set_type(struct enet_tx_descriptor *desc, uint16_t type);
void enet_tx_frame_set_length(struct enet_tx_descriptor *desc, uint16_t length);
uint8_t *enet_tx_frame_get_payload_ptr(struct enet_tx_descriptor *desc);
void enet_send_frame(struct enet_tx_descriptor *desc);

struct enet_rx_descriptor;
bool enet_get_next_available_frame_rx(struct enet_rx_descriptor **desc);
void enet_rx_frame_get_dest(struct enet_rx_descriptor *desc, uint8_t address[6]);
void enet_rx_frame_get_source(struct enet_rx_descriptor *desc, uint8_t address[6]);
uint16_t enet_rx_frame_get_type(struct enet_rx_descriptor *desc);
uint8_t *enet_rx_frame_get_payload_ptr(struct enet_rx_descriptor *desc);
uint16_t enet_rx_frame_get_length(struct enet_rx_descriptor *desc);
void enet_release_frame_rx(struct enet_rx_descriptor *desc);

#endif
