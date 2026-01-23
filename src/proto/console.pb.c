#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "viapb-private.h"
#include "console.pb.h"

// Types from console.proto

static bool pb_decode_enum_viaems_console_SensorFault(viaems_console_SensorFault *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

static bool pb_decode_enum_viaems_console_DecoderLossReason(viaems_console_DecoderLossReason *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

unsigned pb_sizeof_viaems_console_Header(const struct viaems_console_Header *msg) {
  unsigned size = 0;
  if (msg->seq != 0) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->timestamp != 0) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Header(const struct viaems_console_Header *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->seq != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->seq);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->timestamp != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->timestamp);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Header_to_buffer(uint8_t buffer[10], const struct viaems_console_Header *msg) {
  uint8_t *ptr = buffer;
  if (msg->seq != 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->seq);
  }

  if (msg->timestamp != 0) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->timestamp);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Header(struct viaems_console_Header *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_fixed32(&msg->seq, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_fixed32(&msg->timestamp, r, user)) { return false; }
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Sensors(const struct viaems_console_Sensors *msg) {
  unsigned size = 0;
  if (msg->map != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->iat != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->clt != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->brv != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->tps != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->aap != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->frt != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->ego != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->frp != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->eth != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->knock1 != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->knock2 != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->map_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->map_fault);
    size += element_size;
  }

  if (msg->iat_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->iat_fault);
    size += element_size;
  }

  if (msg->clt_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->clt_fault);
    size += element_size;
  }

  if (msg->brv_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->brv_fault);
    size += element_size;
  }

  if (msg->tps_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->tps_fault);
    size += element_size;
  }

  if (msg->aap_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->aap_fault);
    size += element_size;
  }

  if (msg->frt_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->frt_fault);
    size += element_size;
  }

  if (msg->ego_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->ego_fault);
    size += element_size;
  }

  if (msg->frp_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->frp_fault);
    size += element_size;
  }

  if (msg->eth_fault != 0) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->eth_fault);
    size += element_size;
  }

  if (msg->map_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->iat_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->clt_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->brv_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->tps_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->aap_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->frt_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->ego_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->frp_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->eth_rate != 0.0f) {
    size += 2;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Sensors(const struct viaems_console_Sensors *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->map != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->map);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->iat != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->iat);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->clt != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->clt);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->brv != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->brv);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->tps != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tps);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->aap != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->aap);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frt != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frt);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->ego != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ego);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frp != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frp);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->eth != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->eth);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->knock1 != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->knock1);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->knock2 != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->knock2);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->map_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (32 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->map_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->iat_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (33 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->iat_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->clt_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (34 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->clt_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->brv_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (35 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->brv_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->tps_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (36 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->tps_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->aap_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (37 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->aap_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frt_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (38 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->frt_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->ego_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (39 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->ego_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frp_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (40 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->frp_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->eth_fault != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (41 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->eth_fault);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->map_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (64 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->map_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->iat_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (65 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->iat_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->clt_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (66 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->clt_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->brv_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (67 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->brv_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->tps_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (68 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tps_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->aap_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (69 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->aap_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frt_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (70 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frt_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->ego_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (71 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ego_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->frp_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (72 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frp_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->eth_rate != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (73 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->eth_rate);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Sensors_to_buffer(uint8_t buffer[150], const struct viaems_console_Sensors *msg) {
  uint8_t *ptr = buffer;
  if (msg->map != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->map);
  }

  if (msg->iat != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->iat);
  }

  if (msg->clt != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->clt);
  }

  if (msg->brv != 0.0f) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->brv);
  }

  if (msg->tps != 0.0f) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tps);
  }

  if (msg->aap != 0.0f) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->aap);
  }

  if (msg->frt != 0.0f) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frt);
  }

  if (msg->ego != 0.0f) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ego);
  }

  if (msg->frp != 0.0f) {
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frp);
  }

  if (msg->eth != 0.0f) {
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->eth);
  }

  if (msg->knock1 != 0.0f) {
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->knock1);
  }

  if (msg->knock2 != 0.0f) {
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->knock2);
  }

  if (msg->map_fault != 0) {
    ptr += pb_encode_varint(ptr, (32 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->map_fault);
  }

  if (msg->iat_fault != 0) {
    ptr += pb_encode_varint(ptr, (33 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->iat_fault);
  }

  if (msg->clt_fault != 0) {
    ptr += pb_encode_varint(ptr, (34 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->clt_fault);
  }

  if (msg->brv_fault != 0) {
    ptr += pb_encode_varint(ptr, (35 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->brv_fault);
  }

  if (msg->tps_fault != 0) {
    ptr += pb_encode_varint(ptr, (36 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->tps_fault);
  }

  if (msg->aap_fault != 0) {
    ptr += pb_encode_varint(ptr, (37 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->aap_fault);
  }

  if (msg->frt_fault != 0) {
    ptr += pb_encode_varint(ptr, (38 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->frt_fault);
  }

  if (msg->ego_fault != 0) {
    ptr += pb_encode_varint(ptr, (39 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->ego_fault);
  }

  if (msg->frp_fault != 0) {
    ptr += pb_encode_varint(ptr, (40 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->frp_fault);
  }

  if (msg->eth_fault != 0) {
    ptr += pb_encode_varint(ptr, (41 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->eth_fault);
  }

  if (msg->map_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (64 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->map_rate);
  }

  if (msg->iat_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (65 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->iat_rate);
  }

  if (msg->clt_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (66 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->clt_rate);
  }

  if (msg->brv_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (67 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->brv_rate);
  }

  if (msg->tps_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (68 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tps_rate);
  }

  if (msg->aap_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (69 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->aap_rate);
  }

  if (msg->frt_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (70 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frt_rate);
  }

  if (msg->ego_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (71 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ego_rate);
  }

  if (msg->frp_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (72 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frp_rate);
  }

  if (msg->eth_rate != 0.0f) {
    ptr += pb_encode_varint(ptr, (73 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->eth_rate);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Sensors(struct viaems_console_Sensors *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->map, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->iat, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->clt, r, user)) { return false; }
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->brv, r, user)) { return false; }
    }
    if (prefix == ((5ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->tps, r, user)) { return false; }
    }
    if (prefix == ((6ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->aap, r, user)) { return false; }
    }
    if (prefix == ((7ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->frt, r, user)) { return false; }
    }
    if (prefix == ((8ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->ego, r, user)) { return false; }
    }
    if (prefix == ((9ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->frp, r, user)) { return false; }
    }
    if (prefix == ((10ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->eth, r, user)) { return false; }
    }
    if (prefix == ((14ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->knock1, r, user)) { return false; }
    }
    if (prefix == ((15ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->knock2, r, user)) { return false; }
    }
    if (prefix == ((32ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->map_fault, r, user)) { return false; }
    }
    if (prefix == ((33ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->iat_fault, r, user)) { return false; }
    }
    if (prefix == ((34ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->clt_fault, r, user)) { return false; }
    }
    if (prefix == ((35ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->brv_fault, r, user)) { return false; }
    }
    if (prefix == ((36ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->tps_fault, r, user)) { return false; }
    }
    if (prefix == ((37ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->aap_fault, r, user)) { return false; }
    }
    if (prefix == ((38ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->frt_fault, r, user)) { return false; }
    }
    if (prefix == ((39ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->ego_fault, r, user)) { return false; }
    }
    if (prefix == ((40ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->frp_fault, r, user)) { return false; }
    }
    if (prefix == ((41ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_SensorFault(&msg->eth_fault, r, user)) { return false; }
    }
    if (prefix == ((64ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->map_rate, r, user)) { return false; }
    }
    if (prefix == ((65ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->iat_rate, r, user)) { return false; }
    }
    if (prefix == ((66ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->clt_rate, r, user)) { return false; }
    }
    if (prefix == ((67ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->brv_rate, r, user)) { return false; }
    }
    if (prefix == ((68ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->tps_rate, r, user)) { return false; }
    }
    if (prefix == ((69ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->aap_rate, r, user)) { return false; }
    }
    if (prefix == ((70ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->frt_rate, r, user)) { return false; }
    }
    if (prefix == ((71ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->ego_rate, r, user)) { return false; }
    }
    if (prefix == ((72ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->frp_rate, r, user)) { return false; }
    }
    if (prefix == ((73ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->eth_rate, r, user)) { return false; }
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Position(const struct viaems_console_Position *msg) {
  unsigned size = 0;
  if (msg->time != 0) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->valid_before_timestamp != 0) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_position) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->synced) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->loss_cause != 0) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->loss_cause);
    size += element_size;
  }

  if (msg->last_angle != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->instantaneous_rpm != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->average_rpm != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Position(const struct viaems_console_Position *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->time != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->time);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->valid_before_timestamp != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->valid_before_timestamp);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_position) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->has_position);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->synced) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->synced);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->loss_cause != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->loss_cause);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->last_angle != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->last_angle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->instantaneous_rpm != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->instantaneous_rpm);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->average_rpm != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->average_rpm);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Position_to_buffer(uint8_t buffer[31], const struct viaems_console_Position *msg) {
  uint8_t *ptr = buffer;
  if (msg->time != 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->time);
  }

  if (msg->valid_before_timestamp != 0) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_fixed32(ptr, msg->valid_before_timestamp);
  }

  if (msg->has_position) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->has_position);
  }

  if (msg->synced) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->synced);
  }

  if (msg->loss_cause != 0) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->loss_cause);
  }

  if (msg->last_angle != 0.0f) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->last_angle);
  }

  if (msg->instantaneous_rpm != 0.0f) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->instantaneous_rpm);
  }

  if (msg->average_rpm != 0.0f) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->average_rpm);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Position(struct viaems_console_Position *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_fixed32(&msg->time, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_fixed32(&msg->valid_before_timestamp, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->has_position, r, user)) { return false; }
    }
    if (prefix == ((4ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->synced, r, user)) { return false; }
    }
    if (prefix == ((5ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_DecoderLossReason(&msg->loss_cause, r, user)) { return false; }
    }
    if (prefix == ((6ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->last_angle, r, user)) { return false; }
    }
    if (prefix == ((7ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->instantaneous_rpm, r, user)) { return false; }
    }
    if (prefix == ((8ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->average_rpm, r, user)) { return false; }
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Calculations(const struct viaems_console_Calculations *msg) {
  unsigned size = 0;
  if (msg->advance != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->dwell_us != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->fuel_us != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->airmass_per_cycle != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->fuelvol_per_cycle != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->tipin_percent != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->injector_dead_time != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->pulse_width_correction != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->lambda != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->ve != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->engine_temp_enrichment != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->rpm_limit_cut) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->boost_cut) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->fuel_overduty_cut) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->dwell_overduty_cut) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Calculations(const struct viaems_console_Calculations *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->advance != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->advance);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->dwell_us != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->dwell_us);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->fuel_us != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_us);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->airmass_per_cycle != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->airmass_per_cycle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->fuelvol_per_cycle != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuelvol_per_cycle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->tipin_percent != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tipin_percent);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->injector_dead_time != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->injector_dead_time);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->pulse_width_correction != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->pulse_width_correction);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->lambda != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lambda);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->ve != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ve);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->engine_temp_enrichment != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (11 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->engine_temp_enrichment);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->rpm_limit_cut) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (12 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->rpm_limit_cut);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->boost_cut) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (13 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->boost_cut);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->fuel_overduty_cut) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->fuel_overduty_cut);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->dwell_overduty_cut) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->dwell_overduty_cut);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Calculations_to_buffer(uint8_t buffer[63], const struct viaems_console_Calculations *msg) {
  uint8_t *ptr = buffer;
  if (msg->advance != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->advance);
  }

  if (msg->dwell_us != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->dwell_us);
  }

  if (msg->fuel_us != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_us);
  }

  if (msg->airmass_per_cycle != 0.0f) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->airmass_per_cycle);
  }

  if (msg->fuelvol_per_cycle != 0.0f) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuelvol_per_cycle);
  }

  if (msg->tipin_percent != 0.0f) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->tipin_percent);
  }

  if (msg->injector_dead_time != 0.0f) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->injector_dead_time);
  }

  if (msg->pulse_width_correction != 0.0f) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->pulse_width_correction);
  }

  if (msg->lambda != 0.0f) {
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lambda);
  }

  if (msg->ve != 0.0f) {
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ve);
  }

  if (msg->engine_temp_enrichment != 0.0f) {
    ptr += pb_encode_varint(ptr, (11 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->engine_temp_enrichment);
  }

  if (msg->rpm_limit_cut) {
    ptr += pb_encode_varint(ptr, (12 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->rpm_limit_cut);
  }

  if (msg->boost_cut) {
    ptr += pb_encode_varint(ptr, (13 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->boost_cut);
  }

  if (msg->fuel_overduty_cut) {
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->fuel_overduty_cut);
  }

  if (msg->dwell_overduty_cut) {
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->dwell_overduty_cut);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Calculations(struct viaems_console_Calculations *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->advance, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->dwell_us, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fuel_us, r, user)) { return false; }
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->airmass_per_cycle, r, user)) { return false; }
    }
    if (prefix == ((5ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fuelvol_per_cycle, r, user)) { return false; }
    }
    if (prefix == ((6ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->tipin_percent, r, user)) { return false; }
    }
    if (prefix == ((7ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->injector_dead_time, r, user)) { return false; }
    }
    if (prefix == ((8ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->pulse_width_correction, r, user)) { return false; }
    }
    if (prefix == ((9ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->lambda, r, user)) { return false; }
    }
    if (prefix == ((10ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->ve, r, user)) { return false; }
    }
    if (prefix == ((11ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->engine_temp_enrichment, r, user)) { return false; }
    }
    if (prefix == ((12ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->rpm_limit_cut, r, user)) { return false; }
    }
    if (prefix == ((13ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->boost_cut, r, user)) { return false; }
    }
    if (prefix == ((14ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->fuel_overduty_cut, r, user)) { return false; }
    }
    if (prefix == ((15ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->dwell_overduty_cut, r, user)) { return false; }
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Event(const struct viaems_console_Event *msg) {
  unsigned size = 0;
  if (msg->has_header) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Header(&msg->header);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_trigger) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->trigger);
    size += element_size;
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_output_pins) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->output_pins);
    size += element_size;
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_gpio_pins) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->gpio_pins);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Event(const struct viaems_console_Event *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_header) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Header(&msg->header);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Header(&msg->header, w, user)) { return false; }
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_trigger) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->trigger);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_output_pins) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->output_pins);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_gpio_pins) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->gpio_pins);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Event_to_buffer(uint8_t buffer[18], const struct viaems_console_Event *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_header) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Header(&msg->header);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Header_to_buffer(ptr, &msg->header);
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_trigger) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->trigger);
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_output_pins) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->output_pins);
  }

  if (msg->which_type == PB_TAG_viaems_console_Event_gpio_pins) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->gpio_pins);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Event(struct viaems_console_Event *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Header(&msg->header, pb_bounded_read, &br)) { return false; }
      msg->has_header = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->trigger, r, user)) { return false; }
      msg->which_type = 2;
    }
    if (prefix == ((3ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->output_pins, r, user)) { return false; }
      msg->which_type = 3;
    }
    if (prefix == ((4ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->gpio_pins, r, user)) { return false; }
      msg->which_type = 4;
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_EngineUpdate(const struct viaems_console_EngineUpdate *msg) {
  unsigned size = 0;
  if (msg->has_header) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Header(&msg->header);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_position) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Position(&msg->position);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_sensors) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Sensors(&msg->sensors);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_calculations) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Calculations(&msg->calculations);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_EngineUpdate(const struct viaems_console_EngineUpdate *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_header) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Header(&msg->header);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Header(&msg->header, w, user)) { return false; }
  }

  if (msg->has_position) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Position(&msg->position);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Position(&msg->position, w, user)) { return false; }
  }

  if (msg->has_sensors) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Sensors(&msg->sensors);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Sensors(&msg->sensors, w, user)) { return false; }
  }

  if (msg->has_calculations) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Calculations(&msg->calculations);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Calculations(&msg->calculations, w, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_EngineUpdate_to_buffer(uint8_t buffer[263], const struct viaems_console_EngineUpdate *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_header) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Header(&msg->header);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Header_to_buffer(ptr, &msg->header);
  }

  if (msg->has_position) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Position(&msg->position);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Position_to_buffer(ptr, &msg->position);
  }

  if (msg->has_sensors) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Sensors(&msg->sensors);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Sensors_to_buffer(ptr, &msg->sensors);
  }

  if (msg->has_calculations) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Calculations(&msg->calculations);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Calculations_to_buffer(ptr, &msg->calculations);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_EngineUpdate(struct viaems_console_EngineUpdate *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Header(&msg->header, pb_bounded_read, &br)) { return false; }
      msg->has_header = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Position(&msg->position, pb_bounded_read, &br)) { return false; }
      msg->has_position = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Sensors(&msg->sensors, pb_bounded_read, &br)) { return false; }
      msg->has_sensors = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Calculations(&msg->calculations, pb_bounded_read, &br)) { return false; }
      msg->has_calculations = true;
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Configuration_TableRow(const struct viaems_console_Configuration_TableRow *msg) {
  unsigned size = 0;
  if (msg->values_count > 0) {
    unsigned packed_size = msg->values_count * 4;
    size += 1;  // Size of tag
    size += pb_sizeof_varint(packed_size);
    size += packed_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_TableAxis(const struct viaems_console_Configuration_TableAxis *msg) {
  unsigned size = 0;
  if (msg->has_name) {
    size += 1;  // Size of tag
    unsigned element_size = msg->name.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->values_count > 0) {
    unsigned packed_size = msg->values_count * 4;
    size += 1;  // Size of tag
    size += pb_sizeof_varint(packed_size);
    size += packed_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Table1d(const struct viaems_console_Configuration_Table1d *msg) {
  unsigned size = 0;
  if (msg->has_name) {
    size += 1;  // Size of tag
    unsigned element_size = msg->name.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_cols) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_data) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Table2d(const struct viaems_console_Configuration_Table2d *msg) {
  unsigned size = 0;
  if (msg->has_name) {
    size += 1;  // Size of tag
    unsigned element_size = msg->name.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_cols) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_rows) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->rows);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->data_count > 0) {
    for (unsigned i = 0; i < msg->data_count; i++) {
      size += 1;  // Size of tag
      unsigned element_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data[i]);
      size += pb_sizeof_varint(element_size);
      size += element_size;
    }
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Output(const struct viaems_console_Configuration_Output *msg) {
  unsigned size = 0;
  if (msg->has_pin) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->pin);
    size += element_size;
  }

  if (msg->has_type) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->type);
    size += element_size;
  }

  if (msg->has_inverted) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->has_angle) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor_LinearConfig(const struct viaems_console_Configuration_Sensor_LinearConfig *msg) {
  unsigned size = 0;
  if (msg->output_min != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->output_max != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->input_min != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->input_max != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor_ConstConfig(const struct viaems_console_Configuration_Sensor_ConstConfig *msg) {
  unsigned size = 0;
  if (msg->fixed_value != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor_ThermistorConfig(const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg) {
  unsigned size = 0;
  if (msg->a != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->b != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->c != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->bias != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor_FaultConfig(const struct viaems_console_Configuration_Sensor_FaultConfig *msg) {
  unsigned size = 0;
  if (msg->min != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->max != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->value != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor_WindowConfig(const struct viaems_console_Configuration_Sensor_WindowConfig *msg) {
  unsigned size = 0;
  if (msg->capture_width != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->total_width != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->offset != 0.0f) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensor(const struct viaems_console_Configuration_Sensor *msg) {
  unsigned size = 0;
  if (msg->has_source) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->source);
    size += element_size;
  }

  if (msg->has_method) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->method);
    size += element_size;
  }

  if (msg->has_pin) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->pin);
    size += element_size;
  }

  if (msg->has_lag) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_linear_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor_LinearConfig(&msg->linear_config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_const_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor_ConstConfig(&msg->const_config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_thermistor_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor_ThermistorConfig(&msg->thermistor_config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_fault_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor_FaultConfig(&msg->fault_config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_window_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor_WindowConfig(&msg->window_config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_KnockSensor(const struct viaems_console_Configuration_KnockSensor *msg) {
  unsigned size = 0;
  if (msg->has_enabled) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->has_frequency) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_threshold) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Sensors(const struct viaems_console_Configuration_Sensors *msg) {
  unsigned size = 0;
  if (msg->has_aap) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->aap);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_brv) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->brv);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_clt) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->clt);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_ego) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->ego);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_frt) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frt);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_iat) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->iat);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_map) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->map);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_tps) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->tps);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_frp) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frp);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_eth) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->eth);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_knock1) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock1);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_knock2) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock2);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Decoder(const struct viaems_console_Configuration_Decoder *msg) {
  unsigned size = 0;
  if (msg->has_trigger_type) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->trigger_type);
    size += element_size;
  }

  if (msg->has_degrees_per_trigger) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_max_tooth_variance) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_min_rpm) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_num_triggers) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->num_triggers);
    size += element_size;
  }

  if (msg->has_offset) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_TriggerInput(const struct viaems_console_Configuration_TriggerInput *msg) {
  unsigned size = 0;
  if (msg->has_edge) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->edge);
    size += element_size;
  }

  if (msg->has_type) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->type);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_CrankEnrichment(const struct viaems_console_Configuration_CrankEnrichment *msg) {
  unsigned size = 0;
  if (msg->has_cranking_rpm) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_cranking_temp) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_enrich_amt) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Fueling(const struct viaems_console_Configuration_Fueling *msg) {
  unsigned size = 0;
  if (msg->has_fuel_pump_pin) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->fuel_pump_pin);
    size += element_size;
  }

  if (msg->has_cylinder_cc) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_fuel_density) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_fuel_stoich_ratio) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_injections_per_cycle) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->injections_per_cycle);
    size += element_size;
  }

  if (msg->has_injector_cc) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_max_duty_cycle) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_crank_enrich) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_CrankEnrichment(&msg->crank_enrich);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_pulse_width_compensation) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pulse_width_compensation);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_injector_dead_time) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->injector_dead_time);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_engine_temp_enrichment) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->engine_temp_enrichment);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_commanded_lambda) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->commanded_lambda);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_ve) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->ve);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_tipin_enrich_amount) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->tipin_enrich_amount);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_tipin_enrich_duration) {
    size += 2;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->tipin_enrich_duration);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Ignition(const struct viaems_console_Configuration_Ignition *msg) {
  unsigned size = 0;
  if (msg->has_type) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->type);
    size += element_size;
  }

  if (msg->has_fixed_dwell) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_fixed_duty) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_ignitions_per_cycle) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_min_coil_cooldown_us) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->min_coil_cooldown_us);
    size += element_size;
  }

  if (msg->has_min_dwell_us) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->min_dwell_us);
    size += element_size;
  }

  if (msg->has_dwell) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->dwell);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_timing) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->timing);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_BoostControl(const struct viaems_console_Configuration_BoostControl *msg) {
  unsigned size = 0;
  if (msg->has_pin) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->pin);
    size += element_size;
  }

  if (msg->has_control_threshold_map) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_control_threshold_tps) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_enable_threshold_map) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_overboost_map) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_pwm_vs_rpm) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pwm_vs_rpm);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_CheckEngineLight(const struct viaems_console_Configuration_CheckEngineLight *msg) {
  unsigned size = 0;
  if (msg->has_pin) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->pin);
    size += element_size;
  }

  if (msg->has_lean_boost_ego) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_lean_boost_map_enable) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_RpmCut(const struct viaems_console_Configuration_RpmCut *msg) {
  unsigned size = 0;
  if (msg->has_rpm_limit_start) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }

  if (msg->has_rpm_limit_stop) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration_Debug(const struct viaems_console_Configuration_Debug *msg) {
  unsigned size = 0;
  if (msg->has_test_trigger_enabled) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->has_test_trigger_rpm) {
    size += 1;  // Size of tag
    unsigned element_size = 4;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Configuration(const struct viaems_console_Configuration *msg) {
  unsigned size = 0;
  if (msg->outputs_count > 0) {
    for (unsigned i = 0; i < msg->outputs_count; i++) {
      size += 1;  // Size of tag
      unsigned element_size = pb_sizeof_viaems_console_Configuration_Output(&msg->outputs[i]);
      size += pb_sizeof_varint(element_size);
      size += element_size;
    }
  }

  if (msg->triggers_count > 0) {
    for (unsigned i = 0; i < msg->triggers_count; i++) {
      size += 1;  // Size of tag
      unsigned element_size = pb_sizeof_viaems_console_Configuration_TriggerInput(&msg->triggers[i]);
      size += pb_sizeof_varint(element_size);
      size += element_size;
    }
  }

  if (msg->has_sensors) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Sensors(&msg->sensors);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_ignition) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Ignition(&msg->ignition);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_fueling) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Fueling(&msg->fueling);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_decoder) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Decoder(&msg->decoder);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_rpm_cut) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_RpmCut(&msg->rpm_cut);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_cel) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_CheckEngineLight(&msg->cel);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_boost_control) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_BoostControl(&msg->boost_control);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->has_debug) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration_Debug(&msg->debug);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Configuration_TableRow(const struct viaems_console_Configuration_TableRow *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->values_count > 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned sz = 4 * msg->values_count;
    ptr += pb_encode_varint(ptr, sz);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    for (unsigned idx = 0; idx < msg->values_count; idx++) {
      unsigned len = pb_encode_float(scratch, msg->values[idx]);
      if (!w(scratch, len, user)) { return false; }
    }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_TableAxis(const struct viaems_console_Configuration_TableAxis *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_name) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->name.str, msg->name.len, user)) { return false; }
  }

  if (msg->values_count > 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned sz = 4 * msg->values_count;
    ptr += pb_encode_varint(ptr, sz);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    for (unsigned idx = 0; idx < msg->values_count; idx++) {
      unsigned len = pb_encode_float(scratch, msg->values[idx]);
      if (!w(scratch, len, user)) { return false; }
    }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Table1d(const struct viaems_console_Configuration_Table1d *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_name) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->name.str, msg->name.len, user)) { return false; }
  }

  if (msg->has_cols) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_TableAxis(&msg->cols, w, user)) { return false; }
  }

  if (msg->has_data) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_TableRow(&msg->data, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Table2d(const struct viaems_console_Configuration_Table2d *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_name) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->name.str, msg->name.len, user)) { return false; }
  }

  if (msg->has_cols) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_TableAxis(&msg->cols, w, user)) { return false; }
  }

  if (msg->has_rows) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->rows);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_TableAxis(&msg->rows, w, user)) { return false; }
  }

  if (msg->data_count > 0) {
    for (unsigned idx = 0; idx < msg->data_count; idx++) {
      uint8_t *ptr = scratch;
      ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      if (!w(scratch, ptr - scratch, user)) { return false; }
      if (!pb_encode_viaems_console_Configuration_TableRow(&msg->data[idx], w, user)) { return false; }
    }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Output(const struct viaems_console_Configuration_Output *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_pin) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_type) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_inverted) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->inverted);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_angle) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->angle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor_LinearConfig(const struct viaems_console_Configuration_Sensor_LinearConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->output_min != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->output_min);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->output_max != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->output_max);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->input_min != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->input_min);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->input_max != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->input_max);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor_ConstConfig(const struct viaems_console_Configuration_Sensor_ConstConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->fixed_value != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_value);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig(const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->a != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->a);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->b != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->b);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->c != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->c);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->bias != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->bias);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor_FaultConfig(const struct viaems_console_Configuration_Sensor_FaultConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->min != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->min);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->max != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->value != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->value);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor_WindowConfig(const struct viaems_console_Configuration_Sensor_WindowConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->capture_width != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->capture_width);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->total_width != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->total_width);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->offset != 0.0f) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->offset);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensor(const struct viaems_console_Configuration_Sensor *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_source) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->source);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_method) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->method);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_pin) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_lag) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lag);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_linear_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_LinearConfig(&msg->linear_config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor_LinearConfig(&msg->linear_config, w, user)) { return false; }
  }

  if (msg->has_const_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_ConstConfig(&msg->const_config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor_ConstConfig(&msg->const_config, w, user)) { return false; }
  }

  if (msg->has_thermistor_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_ThermistorConfig(&msg->thermistor_config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig(&msg->thermistor_config, w, user)) { return false; }
  }

  if (msg->has_fault_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_FaultConfig(&msg->fault_config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor_FaultConfig(&msg->fault_config, w, user)) { return false; }
  }

  if (msg->has_window_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_WindowConfig(&msg->window_config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor_WindowConfig(&msg->window_config, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_KnockSensor(const struct viaems_console_Configuration_KnockSensor *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_enabled) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->enabled);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_frequency) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frequency);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_threshold) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->threshold);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Sensors(const struct viaems_console_Configuration_Sensors *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_aap) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->aap);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->aap, w, user)) { return false; }
  }

  if (msg->has_brv) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->brv);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->brv, w, user)) { return false; }
  }

  if (msg->has_clt) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->clt);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->clt, w, user)) { return false; }
  }

  if (msg->has_ego) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->ego);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->ego, w, user)) { return false; }
  }

  if (msg->has_frt) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frt);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->frt, w, user)) { return false; }
  }

  if (msg->has_iat) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->iat);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->iat, w, user)) { return false; }
  }

  if (msg->has_map) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->map);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->map, w, user)) { return false; }
  }

  if (msg->has_tps) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->tps);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->tps, w, user)) { return false; }
  }

  if (msg->has_frp) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frp);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->frp, w, user)) { return false; }
  }

  if (msg->has_eth) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->eth);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensor(&msg->eth, w, user)) { return false; }
  }

  if (msg->has_knock1) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock1);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_KnockSensor(&msg->knock1, w, user)) { return false; }
  }

  if (msg->has_knock2) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock2);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_KnockSensor(&msg->knock2, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Decoder(const struct viaems_console_Configuration_Decoder *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_trigger_type) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->trigger_type);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_degrees_per_trigger) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->degrees_per_trigger);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_max_tooth_variance) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max_tooth_variance);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_min_rpm) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->min_rpm);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_num_triggers) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->num_triggers);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_offset) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->offset);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_TriggerInput(const struct viaems_console_Configuration_TriggerInput *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_edge) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->edge);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_type) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_CrankEnrichment(const struct viaems_console_Configuration_CrankEnrichment *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_cranking_rpm) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cranking_rpm);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_cranking_temp) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cranking_temp);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_enrich_amt) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->enrich_amt);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Fueling(const struct viaems_console_Configuration_Fueling *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_fuel_pump_pin) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->fuel_pump_pin);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_cylinder_cc) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cylinder_cc);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_fuel_density) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_density);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_fuel_stoich_ratio) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_stoich_ratio);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_injections_per_cycle) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->injections_per_cycle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_injector_cc) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->injector_cc);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_max_duty_cycle) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max_duty_cycle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_crank_enrich) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (16 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_CrankEnrichment(&msg->crank_enrich);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_CrankEnrichment(&msg->crank_enrich, w, user)) { return false; }
  }

  if (msg->has_pulse_width_compensation) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (17 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pulse_width_compensation);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table1d(&msg->pulse_width_compensation, w, user)) { return false; }
  }

  if (msg->has_injector_dead_time) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (18 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->injector_dead_time);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table1d(&msg->injector_dead_time, w, user)) { return false; }
  }

  if (msg->has_engine_temp_enrichment) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (19 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->engine_temp_enrichment);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table2d(&msg->engine_temp_enrichment, w, user)) { return false; }
  }

  if (msg->has_commanded_lambda) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (20 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->commanded_lambda);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table2d(&msg->commanded_lambda, w, user)) { return false; }
  }

  if (msg->has_ve) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (21 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->ve);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table2d(&msg->ve, w, user)) { return false; }
  }

  if (msg->has_tipin_enrich_amount) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (22 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->tipin_enrich_amount);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table2d(&msg->tipin_enrich_amount, w, user)) { return false; }
  }

  if (msg->has_tipin_enrich_duration) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (23 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->tipin_enrich_duration);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table1d(&msg->tipin_enrich_duration, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Ignition(const struct viaems_console_Configuration_Ignition *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_type) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_fixed_dwell) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_dwell);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_fixed_duty) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_duty);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_ignitions_per_cycle) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ignitions_per_cycle);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_min_coil_cooldown_us) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->min_coil_cooldown_us);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_min_dwell_us) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->min_dwell_us);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_dwell) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->dwell);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table1d(&msg->dwell, w, user)) { return false; }
  }

  if (msg->has_timing) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->timing);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table2d(&msg->timing, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_BoostControl(const struct viaems_console_Configuration_BoostControl *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_pin) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_control_threshold_map) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->control_threshold_map);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_control_threshold_tps) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->control_threshold_tps);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_enable_threshold_map) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->enable_threshold_map);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_overboost_map) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->overboost_map);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_pwm_vs_rpm) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pwm_vs_rpm);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Table1d(&msg->pwm_vs_rpm, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_CheckEngineLight(const struct viaems_console_Configuration_CheckEngineLight *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_pin) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_lean_boost_ego) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lean_boost_ego);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_lean_boost_map_enable) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lean_boost_map_enable);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_RpmCut(const struct viaems_console_Configuration_RpmCut *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_rpm_limit_start) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->rpm_limit_start);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_rpm_limit_stop) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->rpm_limit_stop);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration_Debug(const struct viaems_console_Configuration_Debug *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_test_trigger_enabled) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->test_trigger_enabled);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->has_test_trigger_rpm) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->test_trigger_rpm);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Configuration(const struct viaems_console_Configuration *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->outputs_count > 0) {
    for (unsigned idx = 0; idx < msg->outputs_count; idx++) {
      uint8_t *ptr = scratch;
      ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_Output(&msg->outputs[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      if (!w(scratch, ptr - scratch, user)) { return false; }
      if (!pb_encode_viaems_console_Configuration_Output(&msg->outputs[idx], w, user)) { return false; }
    }
  }

  if (msg->triggers_count > 0) {
    for (unsigned idx = 0; idx < msg->triggers_count; idx++) {
      uint8_t *ptr = scratch;
      ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_TriggerInput(&msg->triggers[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      if (!w(scratch, ptr - scratch, user)) { return false; }
      if (!pb_encode_viaems_console_Configuration_TriggerInput(&msg->triggers[idx], w, user)) { return false; }
    }
  }

  if (msg->has_sensors) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensors(&msg->sensors);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Sensors(&msg->sensors, w, user)) { return false; }
  }

  if (msg->has_ignition) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Ignition(&msg->ignition);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Ignition(&msg->ignition, w, user)) { return false; }
  }

  if (msg->has_fueling) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Fueling(&msg->fueling);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Fueling(&msg->fueling, w, user)) { return false; }
  }

  if (msg->has_decoder) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Decoder(&msg->decoder);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Decoder(&msg->decoder, w, user)) { return false; }
  }

  if (msg->has_rpm_cut) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_RpmCut(&msg->rpm_cut);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_RpmCut(&msg->rpm_cut, w, user)) { return false; }
  }

  if (msg->has_cel) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_CheckEngineLight(&msg->cel);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_CheckEngineLight(&msg->cel, w, user)) { return false; }
  }

  if (msg->has_boost_control) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_BoostControl(&msg->boost_control);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_BoostControl(&msg->boost_control, w, user)) { return false; }
  }

  if (msg->has_debug) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Debug(&msg->debug);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration_Debug(&msg->debug, w, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Configuration_TableRow_to_buffer(uint8_t buffer[98], const struct viaems_console_Configuration_TableRow *msg) {
  uint8_t *ptr = buffer;
  if (msg->values_count > 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned sz = 4 * msg->values_count;
    ptr += pb_encode_varint(ptr, sz);
    for (unsigned idx = 0; idx < msg->values_count; idx++) {
      ptr += pb_encode_float(ptr, msg->values[idx]);
    }
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_TableAxis_to_buffer(uint8_t buffer[124], const struct viaems_console_Configuration_TableAxis *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_name) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->name.str, msg->name.len);
    ptr += msg->name.len;
  }

  if (msg->values_count > 0) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned sz = 4 * msg->values_count;
    ptr += pb_encode_varint(ptr, sz);
    for (unsigned idx = 0; idx < msg->values_count; idx++) {
      ptr += pb_encode_float(ptr, msg->values[idx]);
    }
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Table1d_to_buffer(uint8_t buffer[252], const struct viaems_console_Configuration_Table1d *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_name) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->name.str, msg->name.len);
    ptr += msg->name.len;
  }

  if (msg->has_cols) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_TableAxis_to_buffer(ptr, &msg->cols);
  }

  if (msg->has_data) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_TableRow_to_buffer(ptr, &msg->data);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Table2d_to_buffer(uint8_t buffer[2678], const struct viaems_console_Configuration_Table2d *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_name) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->name.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->name.str, msg->name.len);
    ptr += msg->name.len;
  }

  if (msg->has_cols) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->cols);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_TableAxis_to_buffer(ptr, &msg->cols);
  }

  if (msg->has_rows) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableAxis(&msg->rows);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_TableAxis_to_buffer(ptr, &msg->rows);
  }

  if (msg->data_count > 0) {
    for (unsigned idx = 0; idx < msg->data_count; idx++) {
      ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_TableRow(&msg->data[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      ptr += pb_encode_viaems_console_Configuration_TableRow_to_buffer(ptr, &msg->data[idx]);
    }
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Output_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Output *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_pin) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
  }

  if (msg->has_type) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
  }

  if (msg->has_inverted) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->inverted);
  }

  if (msg->has_angle) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->angle);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_LinearConfig_to_buffer(uint8_t buffer[20], const struct viaems_console_Configuration_Sensor_LinearConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->output_min != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->output_min);
  }

  if (msg->output_max != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->output_max);
  }

  if (msg->input_min != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->input_min);
  }

  if (msg->input_max != 0.0f) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->input_max);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_ConstConfig_to_buffer(uint8_t buffer[5], const struct viaems_console_Configuration_Sensor_ConstConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->fixed_value != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_value);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig_to_buffer(uint8_t buffer[20], const struct viaems_console_Configuration_Sensor_ThermistorConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->a != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->a);
  }

  if (msg->b != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->b);
  }

  if (msg->c != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->c);
  }

  if (msg->bias != 0.0f) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->bias);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_FaultConfig_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Sensor_FaultConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->min != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->min);
  }

  if (msg->max != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max);
  }

  if (msg->value != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->value);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_WindowConfig_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_Sensor_WindowConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->capture_width != 0.0f) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->capture_width);
  }

  if (msg->total_width != 0.0f) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->total_width);
  }

  if (msg->offset != 0.0f) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->offset);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensor_to_buffer(uint8_t buffer[100], const struct viaems_console_Configuration_Sensor *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_source) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->source);
  }

  if (msg->has_method) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->method);
  }

  if (msg->has_pin) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
  }

  if (msg->has_lag) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lag);
  }

  if (msg->has_linear_config) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_LinearConfig(&msg->linear_config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_LinearConfig_to_buffer(ptr, &msg->linear_config);
  }

  if (msg->has_const_config) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_ConstConfig(&msg->const_config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_ConstConfig_to_buffer(ptr, &msg->const_config);
  }

  if (msg->has_thermistor_config) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_ThermistorConfig(&msg->thermistor_config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_ThermistorConfig_to_buffer(ptr, &msg->thermistor_config);
  }

  if (msg->has_fault_config) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_FaultConfig(&msg->fault_config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_FaultConfig_to_buffer(ptr, &msg->fault_config);
  }

  if (msg->has_window_config) {
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor_WindowConfig(&msg->window_config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_WindowConfig_to_buffer(ptr, &msg->window_config);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_KnockSensor_to_buffer(uint8_t buffer[12], const struct viaems_console_Configuration_KnockSensor *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_enabled) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->enabled);
  }

  if (msg->has_frequency) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->frequency);
  }

  if (msg->has_threshold) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->threshold);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Sensors_to_buffer(uint8_t buffer[1048], const struct viaems_console_Configuration_Sensors *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_aap) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->aap);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->aap);
  }

  if (msg->has_brv) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->brv);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->brv);
  }

  if (msg->has_clt) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->clt);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->clt);
  }

  if (msg->has_ego) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->ego);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->ego);
  }

  if (msg->has_frt) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frt);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->frt);
  }

  if (msg->has_iat) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->iat);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->iat);
  }

  if (msg->has_map) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->map);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->map);
  }

  if (msg->has_tps) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->tps);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->tps);
  }

  if (msg->has_frp) {
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->frp);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->frp);
  }

  if (msg->has_eth) {
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensor(&msg->eth);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensor_to_buffer(ptr, &msg->eth);
  }

  if (msg->has_knock1) {
    ptr += pb_encode_varint(ptr, (14 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock1);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_KnockSensor_to_buffer(ptr, &msg->knock1);
  }

  if (msg->has_knock2) {
    ptr += pb_encode_varint(ptr, (15 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_KnockSensor(&msg->knock2);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_KnockSensor_to_buffer(ptr, &msg->knock2);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Decoder_to_buffer(uint8_t buffer[28], const struct viaems_console_Configuration_Decoder *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_trigger_type) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->trigger_type);
  }

  if (msg->has_degrees_per_trigger) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->degrees_per_trigger);
  }

  if (msg->has_max_tooth_variance) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max_tooth_variance);
  }

  if (msg->has_min_rpm) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->min_rpm);
  }

  if (msg->has_num_triggers) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->num_triggers);
  }

  if (msg->has_offset) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->offset);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_TriggerInput_to_buffer(uint8_t buffer[4], const struct viaems_console_Configuration_TriggerInput *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_edge) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->edge);
  }

  if (msg->has_type) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_CrankEnrichment_to_buffer(uint8_t buffer[15], const struct viaems_console_Configuration_CrankEnrichment *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_cranking_rpm) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cranking_rpm);
  }

  if (msg->has_cranking_temp) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cranking_temp);
  }

  if (msg->has_enrich_amt) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->enrich_amt);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Fueling_to_buffer(uint8_t buffer[11551], const struct viaems_console_Configuration_Fueling *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_fuel_pump_pin) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->fuel_pump_pin);
  }

  if (msg->has_cylinder_cc) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->cylinder_cc);
  }

  if (msg->has_fuel_density) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_density);
  }

  if (msg->has_fuel_stoich_ratio) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fuel_stoich_ratio);
  }

  if (msg->has_injections_per_cycle) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->injections_per_cycle);
  }

  if (msg->has_injector_cc) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->injector_cc);
  }

  if (msg->has_max_duty_cycle) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->max_duty_cycle);
  }

  if (msg->has_crank_enrich) {
    ptr += pb_encode_varint(ptr, (16 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_CrankEnrichment(&msg->crank_enrich);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_CrankEnrichment_to_buffer(ptr, &msg->crank_enrich);
  }

  if (msg->has_pulse_width_compensation) {
    ptr += pb_encode_varint(ptr, (17 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pulse_width_compensation);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table1d_to_buffer(ptr, &msg->pulse_width_compensation);
  }

  if (msg->has_injector_dead_time) {
    ptr += pb_encode_varint(ptr, (18 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->injector_dead_time);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table1d_to_buffer(ptr, &msg->injector_dead_time);
  }

  if (msg->has_engine_temp_enrichment) {
    ptr += pb_encode_varint(ptr, (19 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->engine_temp_enrichment);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table2d_to_buffer(ptr, &msg->engine_temp_enrichment);
  }

  if (msg->has_commanded_lambda) {
    ptr += pb_encode_varint(ptr, (20 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->commanded_lambda);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table2d_to_buffer(ptr, &msg->commanded_lambda);
  }

  if (msg->has_ve) {
    ptr += pb_encode_varint(ptr, (21 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->ve);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table2d_to_buffer(ptr, &msg->ve);
  }

  if (msg->has_tipin_enrich_amount) {
    ptr += pb_encode_varint(ptr, (22 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->tipin_enrich_amount);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table2d_to_buffer(ptr, &msg->tipin_enrich_amount);
  }

  if (msg->has_tipin_enrich_duration) {
    ptr += pb_encode_varint(ptr, (23 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->tipin_enrich_duration);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table1d_to_buffer(ptr, &msg->tipin_enrich_duration);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Ignition_to_buffer(uint8_t buffer[2965], const struct viaems_console_Configuration_Ignition *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_type) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->type);
  }

  if (msg->has_fixed_dwell) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_dwell);
  }

  if (msg->has_fixed_duty) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->fixed_duty);
  }

  if (msg->has_ignitions_per_cycle) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->ignitions_per_cycle);
  }

  if (msg->has_min_coil_cooldown_us) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->min_coil_cooldown_us);
  }

  if (msg->has_min_dwell_us) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->min_dwell_us);
  }

  if (msg->has_dwell) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->dwell);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table1d_to_buffer(ptr, &msg->dwell);
  }

  if (msg->has_timing) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table2d(&msg->timing);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table2d_to_buffer(ptr, &msg->timing);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_BoostControl_to_buffer(uint8_t buffer[281], const struct viaems_console_Configuration_BoostControl *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_pin) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
  }

  if (msg->has_control_threshold_map) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->control_threshold_map);
  }

  if (msg->has_control_threshold_tps) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->control_threshold_tps);
  }

  if (msg->has_enable_threshold_map) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->enable_threshold_map);
  }

  if (msg->has_overboost_map) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->overboost_map);
  }

  if (msg->has_pwm_vs_rpm) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Table1d(&msg->pwm_vs_rpm);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Table1d_to_buffer(ptr, &msg->pwm_vs_rpm);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_CheckEngineLight_to_buffer(uint8_t buffer[16], const struct viaems_console_Configuration_CheckEngineLight *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_pin) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->pin);
  }

  if (msg->has_lean_boost_ego) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lean_boost_ego);
  }

  if (msg->has_lean_boost_map_enable) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->lean_boost_map_enable);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_RpmCut_to_buffer(uint8_t buffer[10], const struct viaems_console_Configuration_RpmCut *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_rpm_limit_start) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->rpm_limit_start);
  }

  if (msg->has_rpm_limit_stop) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->rpm_limit_stop);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_Debug_to_buffer(uint8_t buffer[7], const struct viaems_console_Configuration_Debug *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_test_trigger_enabled) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->test_trigger_enabled);
  }

  if (msg->has_test_trigger_rpm) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_32BIT);
    ptr += pb_encode_float(ptr, msg->test_trigger_rpm);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Configuration_to_buffer(uint8_t buffer[16222], const struct viaems_console_Configuration *msg) {
  uint8_t *ptr = buffer;
  if (msg->outputs_count > 0) {
    for (unsigned idx = 0; idx < msg->outputs_count; idx++) {
      ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_Output(&msg->outputs[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      ptr += pb_encode_viaems_console_Configuration_Output_to_buffer(ptr, &msg->outputs[idx]);
    }
  }

  if (msg->triggers_count > 0) {
    for (unsigned idx = 0; idx < msg->triggers_count; idx++) {
      ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
      unsigned elem_size = pb_sizeof_viaems_console_Configuration_TriggerInput(&msg->triggers[idx]);
      ptr += pb_encode_varint(ptr, elem_size);
      ptr += pb_encode_viaems_console_Configuration_TriggerInput_to_buffer(ptr, &msg->triggers[idx]);
    }
  }

  if (msg->has_sensors) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Sensors(&msg->sensors);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Sensors_to_buffer(ptr, &msg->sensors);
  }

  if (msg->has_ignition) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Ignition(&msg->ignition);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Ignition_to_buffer(ptr, &msg->ignition);
  }

  if (msg->has_fueling) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Fueling(&msg->fueling);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Fueling_to_buffer(ptr, &msg->fueling);
  }

  if (msg->has_decoder) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Decoder(&msg->decoder);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Decoder_to_buffer(ptr, &msg->decoder);
  }

  if (msg->has_rpm_cut) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_RpmCut(&msg->rpm_cut);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_RpmCut_to_buffer(ptr, &msg->rpm_cut);
  }

  if (msg->has_cel) {
    ptr += pb_encode_varint(ptr, (8 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_CheckEngineLight(&msg->cel);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_CheckEngineLight_to_buffer(ptr, &msg->cel);
  }

  if (msg->has_boost_control) {
    ptr += pb_encode_varint(ptr, (9 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_BoostControl(&msg->boost_control);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_BoostControl_to_buffer(ptr, &msg->boost_control);
  }

  if (msg->has_debug) {
    ptr += pb_encode_varint(ptr, (10 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration_Debug(&msg->debug);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_Debug_to_buffer(ptr, &msg->debug);
  }

  return (ptr - buffer);
}
static bool pb_decode_enum_viaems_console_Configuration_SensorSource(viaems_console_Configuration_SensorSource *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

static bool pb_decode_enum_viaems_console_Configuration_SensorMethod(viaems_console_Configuration_SensorMethod *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

static bool pb_decode_enum_viaems_console_Configuration_TriggerType(viaems_console_Configuration_TriggerType *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

static bool pb_decode_enum_viaems_console_Configuration_InputEdge(viaems_console_Configuration_InputEdge *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

static bool pb_decode_enum_viaems_console_Configuration_InputType(viaems_console_Configuration_InputType *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

bool pb_decode_viaems_console_Configuration_TableRow(struct viaems_console_Configuration_TableRow *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      while (br.len  > 0) {
        if (msg->values_count >= 24) { return false; }
        if (!pb_decode_float(&msg->values[msg->values_count], pb_bounded_read, &br)) { return false; }
        msg->values_count++;
      }
    }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (msg->values_count >= 24) { return false; }
      if (!pb_decode_float(&msg->values[msg->values_count], r, user)) { return false; }
      msg->values_count++;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_TableAxis(struct viaems_console_Configuration_TableAxis *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 24) { return false; }
      if (!r((uint8_t *)msg->name.str, length, user)) { return false; }
      msg->name.len = length;
      msg->has_name = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      while (br.len  > 0) {
        if (msg->values_count >= 24) { return false; }
        if (!pb_decode_float(&msg->values[msg->values_count], pb_bounded_read, &br)) { return false; }
        msg->values_count++;
      }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (msg->values_count >= 24) { return false; }
      if (!pb_decode_float(&msg->values[msg->values_count], r, user)) { return false; }
      msg->values_count++;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Table1d(struct viaems_console_Configuration_Table1d *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 24) { return false; }
      if (!r((uint8_t *)msg->name.str, length, user)) { return false; }
      msg->name.len = length;
      msg->has_name = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TableAxis(&msg->cols, pb_bounded_read, &br)) { return false; }
      msg->has_cols = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TableRow(&msg->data, pb_bounded_read, &br)) { return false; }
      msg->has_data = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Table2d(struct viaems_console_Configuration_Table2d *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 24) { return false; }
      if (!r((uint8_t *)msg->name.str, length, user)) { return false; }
      msg->name.len = length;
      msg->has_name = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TableAxis(&msg->cols, pb_bounded_read, &br)) { return false; }
      msg->has_cols = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TableAxis(&msg->rows, pb_bounded_read, &br)) { return false; }
      msg->has_rows = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 98) { return false; }
      if (msg->data_count >= 24) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TableRow(&msg->data[msg->data_count], pb_bounded_read, &br)) { return false; }
      msg->data_count++;
    }
  }
  return true;
}
static bool pb_decode_enum_viaems_console_Configuration_Output_OutputType(viaems_console_Configuration_Output_OutputType *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

bool pb_decode_viaems_console_Configuration_Output(struct viaems_console_Configuration_Output *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->pin, r, user)) { return false; }
      msg->has_pin = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_Output_OutputType(&msg->type, r, user)) { return false; }
      msg->has_type = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->inverted, r, user)) { return false; }
      msg->has_inverted = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->angle, r, user)) { return false; }
      msg->has_angle = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor_LinearConfig(struct viaems_console_Configuration_Sensor_LinearConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->output_min, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->output_max, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->input_min, r, user)) { return false; }
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->input_max, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor_ConstConfig(struct viaems_console_Configuration_Sensor_ConstConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fixed_value, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor_ThermistorConfig(struct viaems_console_Configuration_Sensor_ThermistorConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->a, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->b, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->c, r, user)) { return false; }
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->bias, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor_FaultConfig(struct viaems_console_Configuration_Sensor_FaultConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->min, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->max, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->value, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor_WindowConfig(struct viaems_console_Configuration_Sensor_WindowConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->capture_width, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->total_width, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->offset, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensor(struct viaems_console_Configuration_Sensor *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_SensorSource(&msg->source, r, user)) { return false; }
      msg->has_source = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_SensorMethod(&msg->method, r, user)) { return false; }
      msg->has_method = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->pin, r, user)) { return false; }
      msg->has_pin = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->lag, r, user)) { return false; }
      msg->has_lag = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor_LinearConfig(&msg->linear_config, pb_bounded_read, &br)) { return false; }
      msg->has_linear_config = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor_ConstConfig(&msg->const_config, pb_bounded_read, &br)) { return false; }
      msg->has_const_config = true;
    }
    if (prefix == ((7ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor_ThermistorConfig(&msg->thermistor_config, pb_bounded_read, &br)) { return false; }
      msg->has_thermistor_config = true;
    }
    if (prefix == ((8ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor_FaultConfig(&msg->fault_config, pb_bounded_read, &br)) { return false; }
      msg->has_fault_config = true;
    }
    if (prefix == ((9ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor_WindowConfig(&msg->window_config, pb_bounded_read, &br)) { return false; }
      msg->has_window_config = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_KnockSensor(struct viaems_console_Configuration_KnockSensor *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->enabled, r, user)) { return false; }
      msg->has_enabled = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->frequency, r, user)) { return false; }
      msg->has_frequency = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->threshold, r, user)) { return false; }
      msg->has_threshold = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Sensors(struct viaems_console_Configuration_Sensors *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->aap, pb_bounded_read, &br)) { return false; }
      msg->has_aap = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->brv, pb_bounded_read, &br)) { return false; }
      msg->has_brv = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->clt, pb_bounded_read, &br)) { return false; }
      msg->has_clt = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->ego, pb_bounded_read, &br)) { return false; }
      msg->has_ego = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->frt, pb_bounded_read, &br)) { return false; }
      msg->has_frt = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->iat, pb_bounded_read, &br)) { return false; }
      msg->has_iat = true;
    }
    if (prefix == ((7ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->map, pb_bounded_read, &br)) { return false; }
      msg->has_map = true;
    }
    if (prefix == ((8ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->tps, pb_bounded_read, &br)) { return false; }
      msg->has_tps = true;
    }
    if (prefix == ((9ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->frp, pb_bounded_read, &br)) { return false; }
      msg->has_frp = true;
    }
    if (prefix == ((10ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensor(&msg->eth, pb_bounded_read, &br)) { return false; }
      msg->has_eth = true;
    }
    if (prefix == ((14ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_KnockSensor(&msg->knock1, pb_bounded_read, &br)) { return false; }
      msg->has_knock1 = true;
    }
    if (prefix == ((15ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_KnockSensor(&msg->knock2, pb_bounded_read, &br)) { return false; }
      msg->has_knock2 = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Decoder(struct viaems_console_Configuration_Decoder *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_TriggerType(&msg->trigger_type, r, user)) { return false; }
      msg->has_trigger_type = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->degrees_per_trigger, r, user)) { return false; }
      msg->has_degrees_per_trigger = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->max_tooth_variance, r, user)) { return false; }
      msg->has_max_tooth_variance = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->min_rpm, r, user)) { return false; }
      msg->has_min_rpm = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->num_triggers, r, user)) { return false; }
      msg->has_num_triggers = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->offset, r, user)) { return false; }
      msg->has_offset = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_TriggerInput(struct viaems_console_Configuration_TriggerInput *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_InputEdge(&msg->edge, r, user)) { return false; }
      msg->has_edge = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_InputType(&msg->type, r, user)) { return false; }
      msg->has_type = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_CrankEnrichment(struct viaems_console_Configuration_CrankEnrichment *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->cranking_rpm, r, user)) { return false; }
      msg->has_cranking_rpm = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->cranking_temp, r, user)) { return false; }
      msg->has_cranking_temp = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->enrich_amt, r, user)) { return false; }
      msg->has_enrich_amt = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Fueling(struct viaems_console_Configuration_Fueling *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->fuel_pump_pin, r, user)) { return false; }
      msg->has_fuel_pump_pin = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->cylinder_cc, r, user)) { return false; }
      msg->has_cylinder_cc = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fuel_density, r, user)) { return false; }
      msg->has_fuel_density = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fuel_stoich_ratio, r, user)) { return false; }
      msg->has_fuel_stoich_ratio = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->injections_per_cycle, r, user)) { return false; }
      msg->has_injections_per_cycle = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->injector_cc, r, user)) { return false; }
      msg->has_injector_cc = true;
    }
    if (prefix == ((7ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->max_duty_cycle, r, user)) { return false; }
      msg->has_max_duty_cycle = true;
    }
    if (prefix == ((16ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_CrankEnrichment(&msg->crank_enrich, pb_bounded_read, &br)) { return false; }
      msg->has_crank_enrich = true;
    }
    if (prefix == ((17ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table1d(&msg->pulse_width_compensation, pb_bounded_read, &br)) { return false; }
      msg->has_pulse_width_compensation = true;
    }
    if (prefix == ((18ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table1d(&msg->injector_dead_time, pb_bounded_read, &br)) { return false; }
      msg->has_injector_dead_time = true;
    }
    if (prefix == ((19ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table2d(&msg->engine_temp_enrichment, pb_bounded_read, &br)) { return false; }
      msg->has_engine_temp_enrichment = true;
    }
    if (prefix == ((20ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table2d(&msg->commanded_lambda, pb_bounded_read, &br)) { return false; }
      msg->has_commanded_lambda = true;
    }
    if (prefix == ((21ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table2d(&msg->ve, pb_bounded_read, &br)) { return false; }
      msg->has_ve = true;
    }
    if (prefix == ((22ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table2d(&msg->tipin_enrich_amount, pb_bounded_read, &br)) { return false; }
      msg->has_tipin_enrich_amount = true;
    }
    if (prefix == ((23ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table1d(&msg->tipin_enrich_duration, pb_bounded_read, &br)) { return false; }
      msg->has_tipin_enrich_duration = true;
    }
  }
  return true;
}
static bool pb_decode_enum_viaems_console_Configuration_Ignition_DwellType(viaems_console_Configuration_Ignition_DwellType *value, pb_read_fn r, void *user) {
  uint32_t temp;
  if (!pb_decode_varint_uint32(&temp, r, user)) { return false; }
  *value = temp;
  return true;
}

bool pb_decode_viaems_console_Configuration_Ignition(struct viaems_console_Configuration_Ignition *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_enum_viaems_console_Configuration_Ignition_DwellType(&msg->type, r, user)) { return false; }
      msg->has_type = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fixed_dwell, r, user)) { return false; }
      msg->has_fixed_dwell = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->fixed_duty, r, user)) { return false; }
      msg->has_fixed_duty = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->ignitions_per_cycle, r, user)) { return false; }
      msg->has_ignitions_per_cycle = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->min_coil_cooldown_us, r, user)) { return false; }
      msg->has_min_coil_cooldown_us = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->min_dwell_us, r, user)) { return false; }
      msg->has_min_dwell_us = true;
    }
    if (prefix == ((7ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table1d(&msg->dwell, pb_bounded_read, &br)) { return false; }
      msg->has_dwell = true;
    }
    if (prefix == ((8ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table2d(&msg->timing, pb_bounded_read, &br)) { return false; }
      msg->has_timing = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_BoostControl(struct viaems_console_Configuration_BoostControl *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->pin, r, user)) { return false; }
      msg->has_pin = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->control_threshold_map, r, user)) { return false; }
      msg->has_control_threshold_map = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->control_threshold_tps, r, user)) { return false; }
      msg->has_control_threshold_tps = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->enable_threshold_map, r, user)) { return false; }
      msg->has_enable_threshold_map = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->overboost_map, r, user)) { return false; }
      msg->has_overboost_map = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Table1d(&msg->pwm_vs_rpm, pb_bounded_read, &br)) { return false; }
      msg->has_pwm_vs_rpm = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_CheckEngineLight(struct viaems_console_Configuration_CheckEngineLight *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->pin, r, user)) { return false; }
      msg->has_pin = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->lean_boost_ego, r, user)) { return false; }
      msg->has_lean_boost_ego = true;
    }
    if (prefix == ((3ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->lean_boost_map_enable, r, user)) { return false; }
      msg->has_lean_boost_map_enable = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_RpmCut(struct viaems_console_Configuration_RpmCut *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->rpm_limit_start, r, user)) { return false; }
      msg->has_rpm_limit_start = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->rpm_limit_stop, r, user)) { return false; }
      msg->has_rpm_limit_stop = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration_Debug(struct viaems_console_Configuration_Debug *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->test_trigger_enabled, r, user)) { return false; }
      msg->has_test_trigger_enabled = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_32BIT)) {
      if (!pb_decode_float(&msg->test_trigger_rpm, r, user)) { return false; }
      msg->has_test_trigger_rpm = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Configuration(struct viaems_console_Configuration *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 15) { return false; }
      if (msg->outputs_count >= 16) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Output(&msg->outputs[msg->outputs_count], pb_bounded_read, &br)) { return false; }
      msg->outputs_count++;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 4) { return false; }
      if (msg->triggers_count >= 4) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_TriggerInput(&msg->triggers[msg->triggers_count], pb_bounded_read, &br)) { return false; }
      msg->triggers_count++;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Sensors(&msg->sensors, pb_bounded_read, &br)) { return false; }
      msg->has_sensors = true;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Ignition(&msg->ignition, pb_bounded_read, &br)) { return false; }
      msg->has_ignition = true;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Fueling(&msg->fueling, pb_bounded_read, &br)) { return false; }
      msg->has_fueling = true;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Decoder(&msg->decoder, pb_bounded_read, &br)) { return false; }
      msg->has_decoder = true;
    }
    if (prefix == ((7ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_RpmCut(&msg->rpm_cut, pb_bounded_read, &br)) { return false; }
      msg->has_rpm_cut = true;
    }
    if (prefix == ((8ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_CheckEngineLight(&msg->cel, pb_bounded_read, &br)) { return false; }
      msg->has_cel = true;
    }
    if (prefix == ((9ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_BoostControl(&msg->boost_control, pb_bounded_read, &br)) { return false; }
      msg->has_boost_control = true;
    }
    if (prefix == ((10ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration_Debug(&msg->debug, pb_bounded_read, &br)) { return false; }
      msg->has_debug = true;
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Request_Ping(const struct viaems_console_Request_Ping *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Request_SetConfig(const struct viaems_console_Request_SetConfig *msg) {
  unsigned size = 0;
  if (msg->has_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Request_GetConfig(const struct viaems_console_Request_GetConfig *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Request_FlashConfig(const struct viaems_console_Request_FlashConfig *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Request_ResetToBootloader(const struct viaems_console_Request_ResetToBootloader *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Request_FirmwareInfo(const struct viaems_console_Request_FirmwareInfo *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Request(const struct viaems_console_Request *msg) {
  unsigned size = 0;
  if (msg->id != 0) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->id);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_ping) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_Ping(&msg->ping);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_fwinfo) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_FirmwareInfo(&msg->fwinfo);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_setconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_SetConfig(&msg->setconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_getconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_GetConfig(&msg->getconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_flashconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_FlashConfig(&msg->flashconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_resettobootloader) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request_ResetToBootloader(&msg->resettobootloader);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Request_Ping(const struct viaems_console_Request_Ping *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Request_SetConfig(const struct viaems_console_Request_SetConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration(&msg->config, w, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Request_GetConfig(const struct viaems_console_Request_GetConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Request_FlashConfig(const struct viaems_console_Request_FlashConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Request_ResetToBootloader(const struct viaems_console_Request_ResetToBootloader *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Request_FirmwareInfo(const struct viaems_console_Request_FirmwareInfo *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Request(const struct viaems_console_Request *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->id != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->id);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_ping) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_Ping(&msg->ping);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_Ping(&msg->ping, w, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_fwinfo) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_FirmwareInfo(&msg->fwinfo);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_FirmwareInfo(&msg->fwinfo, w, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_setconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_SetConfig(&msg->setconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_SetConfig(&msg->setconfig, w, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_getconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_GetConfig(&msg->getconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_GetConfig(&msg->getconfig, w, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_flashconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_FlashConfig(&msg->flashconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_FlashConfig(&msg->flashconfig, w, user)) { return false; }
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_resettobootloader) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_ResetToBootloader(&msg->resettobootloader);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request_ResetToBootloader(&msg->resettobootloader, w, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Request_Ping_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_Ping *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_SetConfig_to_buffer(uint8_t buffer[16225], const struct viaems_console_Request_SetConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_config) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_to_buffer(ptr, &msg->config);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_GetConfig_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_GetConfig *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_FlashConfig_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_FlashConfig *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_ResetToBootloader_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_ResetToBootloader *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_FirmwareInfo_to_buffer(uint8_t buffer[0], const struct viaems_console_Request_FirmwareInfo *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Request_to_buffer(uint8_t buffer[16234], const struct viaems_console_Request *msg) {
  uint8_t *ptr = buffer;
  if (msg->id != 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->id);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_ping) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_Ping(&msg->ping);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_Ping_to_buffer(ptr, &msg->ping);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_fwinfo) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_FirmwareInfo(&msg->fwinfo);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_FirmwareInfo_to_buffer(ptr, &msg->fwinfo);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_setconfig) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_SetConfig(&msg->setconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_SetConfig_to_buffer(ptr, &msg->setconfig);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_getconfig) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_GetConfig(&msg->getconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_GetConfig_to_buffer(ptr, &msg->getconfig);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_flashconfig) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_FlashConfig(&msg->flashconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_FlashConfig_to_buffer(ptr, &msg->flashconfig);
  }

  if (msg->which_request == PB_TAG_viaems_console_Request_resettobootloader) {
    ptr += pb_encode_varint(ptr, (7 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request_ResetToBootloader(&msg->resettobootloader);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_ResetToBootloader_to_buffer(ptr, &msg->resettobootloader);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Request_Ping(struct viaems_console_Request_Ping *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Request_SetConfig(struct viaems_console_Request_SetConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration(&msg->config, pb_bounded_read, &br)) { return false; }
      msg->has_config = true;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Request_GetConfig(struct viaems_console_Request_GetConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Request_FlashConfig(struct viaems_console_Request_FlashConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Request_ResetToBootloader(struct viaems_console_Request_ResetToBootloader *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Request_FirmwareInfo(struct viaems_console_Request_FirmwareInfo *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Request(struct viaems_console_Request *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->id, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_Ping(&msg->ping, pb_bounded_read, &br)) { return false; }
      msg->which_request = 2;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_FirmwareInfo(&msg->fwinfo, pb_bounded_read, &br)) { return false; }
      msg->which_request = 3;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_SetConfig(&msg->setconfig, pb_bounded_read, &br)) { return false; }
      msg->which_request = 4;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_GetConfig(&msg->getconfig, pb_bounded_read, &br)) { return false; }
      msg->which_request = 5;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_FlashConfig(&msg->flashconfig, pb_bounded_read, &br)) { return false; }
      msg->which_request = 6;
    }
    if (prefix == ((7ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request_ResetToBootloader(&msg->resettobootloader, pb_bounded_read, &br)) { return false; }
      msg->which_request = 7;
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Response_Pong(const struct viaems_console_Response_Pong *msg) {
  unsigned size = 0;

  return size;
}
unsigned pb_sizeof_viaems_console_Response_SetConfig(const struct viaems_console_Response_SetConfig *msg) {
  unsigned size = 0;
  if (msg->has_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->success) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }

  if (msg->requires_restart) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Response_GetConfig(const struct viaems_console_Response_GetConfig *msg) {
  unsigned size = 0;
  if (msg->has_config) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->needs_flash) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Response_FlashConfig(const struct viaems_console_Response_FlashConfig *msg) {
  unsigned size = 0;
  if (msg->success) {
    size += 1;  // Size of tag
    unsigned element_size = 1;
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Response_FirmwareInfo(const struct viaems_console_Response_FirmwareInfo *msg) {
  unsigned size = 0;
  if (msg->version.len > 0) {
    size += 1;  // Size of tag
    unsigned element_size = msg->version.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->platform.len > 0) {
    size += 1;  // Size of tag
    unsigned element_size = msg->platform.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->proto.len > 0) {
    size += 1;  // Size of tag
    unsigned element_size = msg->proto.len;
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
unsigned pb_sizeof_viaems_console_Response(const struct viaems_console_Response *msg) {
  unsigned size = 0;
  if (msg->id != 0) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_varint(msg->id);
    size += element_size;
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_pong) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response_Pong(&msg->pong);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_fwinfo) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response_FirmwareInfo(&msg->fwinfo);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_setconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response_SetConfig(&msg->setconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_getconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response_GetConfig(&msg->getconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_flashconfig) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response_FlashConfig(&msg->flashconfig);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Response_Pong(const struct viaems_console_Response_Pong *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  return true;
}
bool pb_encode_viaems_console_Response_SetConfig(const struct viaems_console_Response_SetConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration(&msg->config, w, user)) { return false; }
  }

  if (msg->success) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->success);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->requires_restart) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->requires_restart);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Response_GetConfig(const struct viaems_console_Response_GetConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->has_config) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Configuration(&msg->config, w, user)) { return false; }
  }

  if (msg->needs_flash) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->needs_flash);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Response_FlashConfig(const struct viaems_console_Response_FlashConfig *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->success) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->success);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Response_FirmwareInfo(const struct viaems_console_Response_FirmwareInfo *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->version.len > 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->version.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->version.str, msg->version.len, user)) { return false; }
  }

  if (msg->platform.len > 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->platform.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->platform.str, msg->platform.len, user)) { return false; }
  }

  if (msg->proto.len > 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->proto.len;
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w((const uint8_t *)msg->proto.str, msg->proto.len, user)) { return false; }
  }

  return true;
}
bool pb_encode_viaems_console_Response(const struct viaems_console_Response *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  if (msg->id != 0) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->id);
    if (!w(scratch, ptr - scratch, user)) { return false; }
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_pong) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_Pong(&msg->pong);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response_Pong(&msg->pong, w, user)) { return false; }
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_fwinfo) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_FirmwareInfo(&msg->fwinfo);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response_FirmwareInfo(&msg->fwinfo, w, user)) { return false; }
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_setconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_SetConfig(&msg->setconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response_SetConfig(&msg->setconfig, w, user)) { return false; }
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_getconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_GetConfig(&msg->getconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response_GetConfig(&msg->getconfig, w, user)) { return false; }
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_flashconfig) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_FlashConfig(&msg->flashconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response_FlashConfig(&msg->flashconfig, w, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Response_Pong_to_buffer(uint8_t buffer[0], const struct viaems_console_Response_Pong *msg) {
  uint8_t *ptr = buffer;
  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Response_SetConfig_to_buffer(uint8_t buffer[16229], const struct viaems_console_Response_SetConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_config) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_to_buffer(ptr, &msg->config);
  }

  if (msg->success) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->success);
  }

  if (msg->requires_restart) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->requires_restart);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Response_GetConfig_to_buffer(uint8_t buffer[16227], const struct viaems_console_Response_GetConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->has_config) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Configuration(&msg->config);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Configuration_to_buffer(ptr, &msg->config);
  }

  if (msg->needs_flash) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->needs_flash);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Response_FlashConfig_to_buffer(uint8_t buffer[2], const struct viaems_console_Response_FlashConfig *msg) {
  uint8_t *ptr = buffer;
  if (msg->success) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->success);
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Response_FirmwareInfo_to_buffer(uint8_t buffer[16488], const struct viaems_console_Response_FirmwareInfo *msg) {
  uint8_t *ptr = buffer;
  if (msg->version.len > 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->version.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->version.str, msg->version.len);
    ptr += msg->version.len;
  }

  if (msg->platform.len > 0) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->platform.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->platform.str, msg->platform.len);
    ptr += msg->platform.len;
  }

  if (msg->proto.len > 0) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = msg->proto.len;
    ptr += pb_encode_varint(ptr, elem_size);
    memcpy(ptr, msg->proto.str, msg->proto.len);
    ptr += msg->proto.len;
  }

  return (ptr - buffer);
}
unsigned pb_encode_viaems_console_Response_to_buffer(uint8_t buffer[16498], const struct viaems_console_Response *msg) {
  uint8_t *ptr = buffer;
  if (msg->id != 0) {
    ptr += pb_encode_varint(ptr, (1 << 3) | PB_WT_VARINT);
    ptr += pb_encode_varint(ptr, msg->id);
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_pong) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_Pong(&msg->pong);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_Pong_to_buffer(ptr, &msg->pong);
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_fwinfo) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_FirmwareInfo(&msg->fwinfo);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_FirmwareInfo_to_buffer(ptr, &msg->fwinfo);
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_setconfig) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_SetConfig(&msg->setconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_SetConfig_to_buffer(ptr, &msg->setconfig);
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_getconfig) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_GetConfig(&msg->getconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_GetConfig_to_buffer(ptr, &msg->getconfig);
  }

  if (msg->which_response == PB_TAG_viaems_console_Response_flashconfig) {
    ptr += pb_encode_varint(ptr, (6 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response_FlashConfig(&msg->flashconfig);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_FlashConfig_to_buffer(ptr, &msg->flashconfig);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Response_Pong(struct viaems_console_Response_Pong *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
  }
  return true;
}
bool pb_decode_viaems_console_Response_SetConfig(struct viaems_console_Response_SetConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration(&msg->config, pb_bounded_read, &br)) { return false; }
      msg->has_config = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->success, r, user)) { return false; }
    }
    if (prefix == ((3ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->requires_restart, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Response_GetConfig(struct viaems_console_Response_GetConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Configuration(&msg->config, pb_bounded_read, &br)) { return false; }
      msg->has_config = true;
    }
    if (prefix == ((2ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->needs_flash, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Response_FlashConfig(struct viaems_console_Response_FlashConfig *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_bool(&msg->success, r, user)) { return false; }
    }
  }
  return true;
}
bool pb_decode_viaems_console_Response_FirmwareInfo(struct viaems_console_Response_FirmwareInfo *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 64) { return false; }
      if (!r((uint8_t *)msg->version.str, length, user)) { return false; }
      msg->version.len = length;
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 32) { return false; }
      if (!r((uint8_t *)msg->platform.str, length, user)) { return false; }
      msg->platform.len = length;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      if (length > 16384) { return false; }
      if (!r((uint8_t *)msg->proto.str, length, user)) { return false; }
      msg->proto.len = length;
    }
  }
  return true;
}
bool pb_decode_viaems_console_Response(struct viaems_console_Response *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((1ul << 3) | PB_WT_VARINT)) {
      if (!pb_decode_varint_uint32(&msg->id, r, user)) { return false; }
    }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response_Pong(&msg->pong, pb_bounded_read, &br)) { return false; }
      msg->which_response = 2;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response_FirmwareInfo(&msg->fwinfo, pb_bounded_read, &br)) { return false; }
      msg->which_response = 3;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response_SetConfig(&msg->setconfig, pb_bounded_read, &br)) { return false; }
      msg->which_response = 4;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response_GetConfig(&msg->getconfig, pb_bounded_read, &br)) { return false; }
      msg->which_response = 5;
    }
    if (prefix == ((6ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response_FlashConfig(&msg->flashconfig, pb_bounded_read, &br)) { return false; }
      msg->which_response = 6;
    }
  }
  return true;
}
unsigned pb_sizeof_viaems_console_Message(const struct viaems_console_Message *msg) {
  unsigned size = 0;
  if (msg->which_msg == PB_TAG_viaems_console_Message_engine_update) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_EngineUpdate(&msg->engine_update);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_event) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Event(&msg->event);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_request) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Request(&msg->request);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_response) {
    size += 1;  // Size of tag
    unsigned element_size = pb_sizeof_viaems_console_Response(&msg->response);
    size += pb_sizeof_varint(element_size);
    size += element_size;
  }


  return size;
}
bool pb_encode_viaems_console_Message(const struct viaems_console_Message *msg, pb_write_fn w, void *user) {
  uint8_t scratch[20];
  uint8_t submsg[256];
  if (msg->which_msg == PB_TAG_viaems_console_Message_engine_update) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_EngineUpdate(&msg->engine_update);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_EngineUpdate(&msg->engine_update, w, user)) { return false; }
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_event) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_encode_viaems_console_Event_to_buffer(submsg, &msg->event);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!w(submsg, elem_size, user)) { return false; }
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_request) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request(&msg->request);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Request(&msg->request, w, user)) { return false; }
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_response) {
    uint8_t *ptr = scratch;
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response(&msg->response);
    ptr += pb_encode_varint(ptr, elem_size);
    if (!w(scratch, ptr - scratch, user)) { return false; }
    if (!pb_encode_viaems_console_Response(&msg->response, w, user)) { return false; }
  }

  return true;
}
unsigned pb_encode_viaems_console_Message_to_buffer(uint8_t buffer[16502], const struct viaems_console_Message *msg) {
  uint8_t *ptr = buffer;
  if (msg->which_msg == PB_TAG_viaems_console_Message_engine_update) {
    ptr += pb_encode_varint(ptr, (2 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_EngineUpdate(&msg->engine_update);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_EngineUpdate_to_buffer(ptr, &msg->engine_update);
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_event) {
    ptr += pb_encode_varint(ptr, (3 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Event(&msg->event);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Event_to_buffer(ptr, &msg->event);
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_request) {
    ptr += pb_encode_varint(ptr, (4 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Request(&msg->request);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Request_to_buffer(ptr, &msg->request);
  }

  if (msg->which_msg == PB_TAG_viaems_console_Message_response) {
    ptr += pb_encode_varint(ptr, (5 << 3) | PB_WT_STRING);
    unsigned elem_size = pb_sizeof_viaems_console_Response(&msg->response);
    ptr += pb_encode_varint(ptr, elem_size);
    ptr += pb_encode_viaems_console_Response_to_buffer(ptr, &msg->response);
  }

  return (ptr - buffer);
}
bool pb_decode_viaems_console_Message(struct viaems_console_Message *msg, pb_read_fn r, void *user) {
  while (true) {
    uint32_t prefix;
    if (!pb_decode_varint_uint32(&prefix, r, user)) { break; }
    if (prefix == ((2ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_EngineUpdate(&msg->engine_update, pb_bounded_read, &br)) { return false; }
      msg->which_msg = 2;
    }
    if (prefix == ((3ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Event(&msg->event, pb_bounded_read, &br)) { return false; }
      msg->which_msg = 3;
    }
    if (prefix == ((4ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Request(&msg->request, pb_bounded_read, &br)) { return false; }
      msg->which_msg = 4;
    }
    if (prefix == ((5ul << 3) | PB_WT_STRING)) {
      uint32_t length;
      if (!pb_decode_varint_uint32(&length, r, user)) { return false; }
      struct pb_bounded_reader br = { .r = r, .user = user, .len = length };
      if (!pb_decode_viaems_console_Response(&msg->response, pb_bounded_read, &br)) { return false; }
      msg->which_msg = 5;
    }
  }
  return true;
}

