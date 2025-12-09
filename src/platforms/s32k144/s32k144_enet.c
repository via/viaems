#include <memory.h>
#include <string.h>
#include <stdint.h>
#include "s32k1xx.h"

#include "s32k144_enet.h"
#include <stdio.h>

#define MAX_PACKET_SIZE 1536
#define TX_RING_SIZE 4
#define RX_RING_SIZE 4

struct enet_tx_descriptor {
  uint16_t length;
  uint16_t flags;
  uint32_t data;
  uint16_t errors;
  uint16_t flags2;
  uint16_t _reserved[3];
  uint16_t bdu;
  uint32_t timestamp;
  uint16_t _reserved2[4];
} __attribute__((aligned(16), packed));

#define TX_DESC_FLAGS_R     (1u << 15)
#define TX_DESC_FLAGS_W     (1u << 13)
#define TX_DESC_FLAGS_L     (1u << 11)
#define TX_DESC_FLAGS_TC    (1u << 10)
#define TX_DESC_BDU         (1u << 15)

#define RX_DESC_FLAGS_E     (1u << 15)
#define RX_DESC_FLAGS_W     (1u << 13)
#define RX_DESC_FLAGS_L     (1u << 11)
#define RX_DESC_FLAGS_M     (1u << 8)
#define RX_DESC_FLAGS_BC    (1u << 7)
#define RX_DESC_FLAGS_MC    (1u << 6)
#define RX_DESC_FLAGS_LG    (1u << 5)
#define RX_DESC_FLAGS_NO    (1u << 3)
#define RX_DESC_FLAGS_CR    (1u << 2)
#define RX_DESC_FLAGS_OV    (1u << 1)
#define RX_DESC_FLAGS_TR    (1u << 0)
#define RX_DESC_BDU         (1u << 15)

struct enet_rx_descriptor {
  uint16_t length;
  uint16_t flags;
  uint32_t data;
  uint32_t flags2;
  uint16_t checksum;
  uint8_t header_length;
  uint8_t protocol_type;
  uint16_t _reserved;
  uint16_t bdu;
  uint32_t timestamp;
  uint16_t _reserved2[4];
} __attribute__((aligned(16), packed));



static uint8_t tx_packet_memory[TX_RING_SIZE][MAX_PACKET_SIZE] __attribute__((aligned(16))) = { 0 };
static uint8_t rx_packet_memory[RX_RING_SIZE][MAX_PACKET_SIZE] __attribute__((aligned(16))) = { 0 };

static struct enet_tx_descriptor tx_descriptors[TX_RING_SIZE] = { 0 };
static struct enet_rx_descriptor rx_descriptors[RX_RING_SIZE] = { 0 };

static uint32_t next_tx_descriptor = 0;
static uint32_t next_rx_descriptor = 0;

static const uint8_t etheraddr[] = {0xA2, 0x11, 0x22, 0x33, 0x44, 0x55};

void enet_initialize(const struct enet_config *config) {

  *ENET_PAUR = ((uint32_t)etheraddr[4] << 24) |
               ((uint32_t)etheraddr[5] << 16) | 
               0x8808;
  *ENET_PALR = ((uint32_t)etheraddr[0] << 24) |
               ((uint32_t)etheraddr[1] << 16) |
               ((uint32_t)etheraddr[2] << 8)  |
               ((uint32_t)etheraddr[0] << 0); 

  *ENET_RCR = (1u << 8); // MII_MODE = RMII
  *ENET_TCR = (1u << 2) | (1u << 8); // FDEN | ADDINS

  *ENET_ECR = 0xf0000000 | (1u << 8) | (1u << 4); // DBSWP | EN1588
  *ENET_TACC = (1u << 4) | (1u << 3);  // Enable checksum accel
  *ENET_TFWR |= (1u << 8); // Enable store and forward for checksumming

  for (int i = 0; i < TX_RING_SIZE; i++) {
    tx_descriptors[i].data = (uint32_t)&tx_packet_memory[i][0]; 
  }

  for (int i = 0; i < RX_RING_SIZE; i++) {
    rx_descriptors[i].data = (uint32_t)&rx_packet_memory[i][0]; 
    rx_descriptors[i].flags = RX_DESC_FLAGS_E;
  }

  rx_descriptors[RX_RING_SIZE - 1].flags |= RX_DESC_FLAGS_W;
  tx_descriptors[TX_RING_SIZE - 1].flags = TX_DESC_FLAGS_W;

  *ENET_MSCR = (23u << 1); // 1.6 MHz mdio clock with 80 MHz sysclk
  *ENET_MRBR = MAX_PACKET_SIZE;
  *ENET_TDSR = (uint32_t)&tx_descriptors[0];
  *ENET_RDSR = (uint32_t)&rx_descriptors[0];

  *ENET_ECR |= (1u << 1); // Enable
  *ENET_RDAR = (1u << 24); // Let MAC know we've got RX descriptors ready
}


bool enet_get_next_available_frame_tx(struct enet_tx_descriptor **desc) {
  if ((tx_descriptors[next_tx_descriptor].flags & TX_DESC_FLAGS_R) != 0) {
    return false;
  }

  *desc = &tx_descriptors[next_tx_descriptor];
  return true;
}

void enet_tx_frame_set_dest(struct enet_tx_descriptor *desc, uint8_t address[6]) {
  uint8_t *frame = (uint8_t *)desc->data;
  memcpy(frame, address, 6);
}

void enet_tx_frame_set_type(struct enet_tx_descriptor *desc, uint16_t type) {
  uint8_t *frame = (uint8_t *)desc->data;
  frame[12] = (type >> 8);
  frame[13] = (type & 0xFF);
}

void enet_tx_frame_set_length(struct enet_tx_descriptor *desc, uint16_t length) {
  desc->length = length;
}

uint8_t *enet_tx_frame_get_payload_ptr(struct enet_tx_descriptor *desc) {
  uint8_t *frame = (uint8_t *)desc->data + 14;
  return frame;
}

void enet_send_frame(struct enet_tx_descriptor *desc) {
  desc->flags |= (TX_DESC_FLAGS_L) | (TX_DESC_FLAGS_TC);
  desc->bdu &= ~TX_DESC_BDU;

  desc->flags |= (TX_DESC_FLAGS_R);
  next_tx_descriptor += 1;
  if (next_tx_descriptor >= TX_RING_SIZE) {
    next_tx_descriptor = 0;
  }

  *ENET_TDAR = (1u << 24);
}

bool enet_get_next_available_frame_rx(struct enet_rx_descriptor **desc) {
  if ((rx_descriptors[next_rx_descriptor].flags & RX_DESC_FLAGS_E) != 0) {
    return false;
  }

  *desc = &rx_descriptors[next_rx_descriptor];
  return true;
}

void enet_rx_frame_get_dest(struct enet_rx_descriptor *desc, uint8_t address[6]) {
  uint8_t *frame = (uint8_t *)desc->data;
  memcpy(address, frame, 6);
}

void enet_rx_frame_get_source(struct enet_rx_descriptor *desc, uint8_t address[6]) {
  uint8_t *frame = (uint8_t *)desc->data;
  memcpy(address, frame + 6, 6);
}

uint16_t enet_rx_frame_get_type(struct enet_rx_descriptor *desc) {
  uint8_t *frame = (uint8_t *)desc->data;
  return ((uint16_t)frame[12] << 8) | (uint16_t)frame[13];
}

uint8_t *enet_rx_frame_get_payload_ptr(struct enet_rx_descriptor *desc) {
  uint8_t *frame = (uint8_t *)desc->data + 14;
  return frame;
}

uint16_t enet_rx_frame_get_length(struct enet_rx_descriptor *desc) {
  return desc->length;
}

void enet_release_frame_rx(struct enet_rx_descriptor *desc) {
  desc->flags |= RX_DESC_FLAGS_E;
  desc->bdu &= ~RX_DESC_BDU;

  next_rx_descriptor += 1;
  if (next_rx_descriptor >= RX_RING_SIZE) {
    next_rx_descriptor = 0;
  }

  *ENET_RDAR = (1u << 24);
}

/* Needed functionality:
 *  - Statically configured single ethernet MAC address
 *  - Statically configured single IP address
 *  - Statically configured single listening udp port
 *  - Need to receive ARP requests for the configured IP and produce a ARP reply
 *  - Need to receive UDP messages, and produce replies to them, re-using the
 *    source mac address instead of doing an arp exchange
 *  - Need to send UDP messages to multicast address
 */

struct arp {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t op;
  uint8_t sha[6];
  uint32_t spa;
  uint8_t tha[6];
  uint32_t tpa;
};

static struct arp deserialize_arp(uint8_t frame[28]){
  return (struct arp){
    .htype = ((uint16_t)frame[0] << 8) | (uint16_t)frame[1], 
    .ptype = ((uint16_t)frame[2] << 8) | (uint16_t)frame[3], 
    .hlen = frame[4],
    .plen = frame[5],
    .op = ((uint16_t)frame[6] << 8) | (uint16_t)frame[7], 
    .sha = {frame[8], frame[9], frame[10], frame[11], frame[12], frame[13]},
    .spa = ((uint32_t)frame[14] << 24) | 
           ((uint32_t)frame[15] << 16) |
           ((uint32_t)frame[16] << 8) |
           ((uint32_t)frame[17] << 0),
    .tha = {frame[18], frame[19], frame[20], frame[21], frame[22], frame[23]},
    .tpa = ((uint32_t)frame[24] << 24) | 
           ((uint32_t)frame[25] << 16) |
           ((uint32_t)frame[26] << 8) |
           ((uint32_t)frame[27] << 0),
  };
}

void serialize_arp(uint8_t frame[26], const struct arp *arp) {
  frame[0] = arp->htype >> 8;
  frame[1] = arp->htype;

  frame[2] = arp->ptype >> 8;
  frame[3] = arp->ptype;

  frame[4] = arp->hlen;
  frame[5] = arp->plen;

  frame[6] = arp->op >> 8;
  frame[7] = arp->op;

  frame[8] = arp->sha[0];
  frame[9] = arp->sha[1];
  frame[10] = arp->sha[2];
  frame[11] = arp->sha[3];
  frame[12] = arp->sha[4];
  frame[13] = arp->sha[5];

  frame[14] = arp->spa >> 24;
  frame[15] = arp->spa >> 16;
  frame[16] = arp->spa >> 8;
  frame[17] = arp->spa;

  frame[18] = arp->tha[0];
  frame[19] = arp->tha[1];
  frame[20] = arp->tha[2];
  frame[21] = arp->tha[3];
  frame[22] = arp->tha[4];
  frame[23] = arp->tha[5];

  frame[24] = arp->tpa >> 24;
  frame[25] = arp->tpa >> 16;
  frame[26] = arp->tpa >> 8;
  frame[27] = arp->tpa;
}

static bool process_arp_request(struct enet_rx_descriptor *rx) {

  /* Is this an ARP request for our IP? */
  uint16_t length = enet_rx_frame_get_length(rx);
  if (length < 64) {
    return false;
  }

  uint8_t *frame = enet_rx_frame_get_payload_ptr(rx);
  struct arp req = deserialize_arp(frame);
  printf("ARP   HTYPE:%d PTYPE:%d  HLEN:%d PLEN: %d OP:%d\r\n", req.htype, req.ptype, req.hlen, req.plen, req.op);
  printf("      SHA: %x:%x:%x:%x:%x:%x  SPA: %d.%d.%d.%d\r\n",
      req.sha[0], req.sha[1], req.sha[2], req.sha[3], req.sha[4], req.sha[5],
      (req.spa >> 24), (req.spa >> 16) & 0xFF, (req.spa >> 8) & 0xFF, req.spa & 0xFF);
  printf("      THA: %x:%x:%x:%x:%x:%x  TPA: %d.%d.%d.%d\r\n",
      req.tha[0], req.tha[1], req.tha[2], req.tha[3], req.tha[4], req.tha[5],
      (req.tpa >> 24), (req.tpa >> 16) & 0xFF, (req.tpa >> 8) & 0xFF, req.tpa & 0xFF);

  struct enet_tx_descriptor *tx;
  if (!enet_get_next_available_frame_tx(&tx)) {
    return false;
  }
  enet_tx_frame_set_dest(tx, req.sha);
  enet_tx_frame_set_length(tx, enet_rx_frame_get_length(rx));
  enet_tx_frame_set_type(tx, 0x806);
  struct arp reply = req;
  reply.op = 2;
  memcpy(reply.sha, etheraddr, 6);
  reply.spa = req.tpa;
  memcpy(reply.tha, req.sha, 6);
  reply.tpa = req.spa;
  serialize_arp(enet_tx_frame_get_payload_ptr(tx), &reply);
  enet_send_frame(tx);
  return true;
}

void process_ethernet(void) {
  struct enet_rx_descriptor *rx;
  if (enet_get_next_available_frame_rx(&rx)) {
    uint16_t type = enet_rx_frame_get_type(rx);
    if (type == 0x806) { /* EtherType ARP */
      process_arp_request(rx);
    }
    enet_release_frame_rx(rx);
  }
}

struct ipv4_header {
  uint16_t len;
  uint16_t id;


};

struct udp_header {

};

void send_mcast(uint8_t *data, uint32_t length) {
  struct enet_tx_descriptor *tx;
  if (!enet_get_next_available_frame_tx(&tx)) {
    return;
  }
  const uint16_t ip_header_len = 20;
  const uint16_t udp_header_len = 8;
  
  /* TODO make configurable, but for now:
   *  IP: 239.0.0.10
   *  MAC: 01:00:5E:00:00:0A */

  const uint16_t total_len = length + ip_header_len + udp_header_len;
  enet_tx_frame_set_dest(tx, (uint8_t[]){0x01, 0x00, 0x5E, 0x00, 0x00, 0x0A});
  enet_tx_frame_set_length(tx, total_len + 14);
  enet_tx_frame_set_type(tx, 0x800);

  uint8_t *frame = enet_tx_frame_get_payload_ptr(tx);
  frame[0] = (4u << 4) | 5;
  frame[1] = 0;
  frame[2] = total_len >> 8;
  frame[3] = total_len;

  frame[4] = 0;
  frame[5] = 0;
  frame[6] = 0;
  frame[7] = 0;

  frame[8] = 30; // TTL 30
  frame[9] = 17; // UDP
  frame[10] = 0;
  frame[11] = 0;

  frame[12] = 192;
  frame[13] = 168;
  frame[14] = 10;
  frame[15] = 1;

  frame[16] = 239;
  frame[17] = 0;
  frame[18] = 0;
  frame[19] = 10;

  uint16_t sport = 5555;
  uint16_t dport = 5556;
  frame[20] = sport >> 8;
  frame[21] = sport;
  frame[22] = dport >> 8;
  frame[23] = dport;

  uint16_t udp_len = 8 + length;
  frame[24] = udp_len >> 8;
  frame[25] = udp_len;
  frame[26] = 0;
  frame[27] = 0; // udp checksum

  memcpy(&frame[28], data, length);

  enet_send_frame(tx);

}
