#include <stdint.h>

#include "fiber.h"
#include "fiber-private.h"

#include "stdio.h"

/* Uak implementation for cortex-m4/7.
 *
 * MPU regions:
 * 0 - fiber(s) code region 0x08000000 - _etext(08002110) (16K)
 * 1 - fiber(s) data + bss 0x20000000 - _ebss(20009398) (64K)
 * 2 - fiber stack
 */

/* Trace and debug functions:
 * Use the ITM to send trace messages. 
 *
 * 0 - NOTIFY_SET (f)
 * 1 - NOTIFY_WAIT (f)
 * 2 - FIBER_BECOMES_RUNNABLE(f)
 * 3 - FIBER_SUSPEND(f)
 *
 * Prefer single byte ITM payloads:
 * 0 X X X F F F F
 * X - Event type
 * F - First arg (fiber)
 */

enum trace_id {
  NOTIFY_SET = 0,
  NOTIFY_WAIT = 1,
  FIBER_BECOMES_RUNNABLE = 2,
  FIBER_SUSPEND = 3,
};

void emit_trace(enum trace_id _id, uint8_t _arg) {
  volatile uint8_t *STIM0 = (volatile uint8_t *)0xe0000000;
  uint8_t id = ((uint8_t)_id) & 0xf;
  uint8_t arg = ((uint8_t)_arg) & 0xf;
  *STIM0 = (id << 4) | arg;
}


static void hang_forever() {
  while(1);
}

void uak_md_fiber_create(struct fiber *f) {
  /* This platform we just store everything on the fiber's stack, so _md is the
   * stack pointer.  Context frame has R0-R3, R12, LR, PC, PSR.  We also store
   * the other 8 registers after that.
   */

  /* XXX */
  uint32_t *sp = &f->stack[f->stack_size / 4];
  *(sp - 9) = 0xfffffffd;
  *(sp - 8) = 0; /* r0 */
  *(sp - 7) = 1; /* r1 */
  *(sp - 6) = 2; /* r2 */
  *(sp - 5) = 3; /* r3 */
  *(sp - 4) = 12; /* r12 */
  *(sp - 3) = (uint32_t)hang_forever; /* lr */
  *(sp - 2) = (uint32_t)f->entry; /* pc */
  *(sp - 1) = 0x21000000; /* psr */
  f->_md = sp - 17;

  for (int i = 0; i < 8; i++) {
    f->_mpu_context.regions[i].rbar = (1 << 4) | i;
    f->_mpu_context.regions[i].rasr = 0;
  }
}

static void uak_md_set_return_value(struct fiber *f, uint32_t value) {
  uint32_t *sp = f->_md;
  sp[9] = value; /* Set r0 */
}


void uak_md_request_reschedule() {
  volatile uint32_t *ICSR = (volatile uint32_t *)0x0E000ED04;
  uint32_t ICSR_PENDSVSET = (1ULL << 28);
  *ICSR = ICSR_PENDSVSET;
}

static uint32_t mpu_rasr_size(size_t size) {
  uint32_t power = 31 - __builtin_clz(size);
  uint32_t asr_size = power - 1;
  return asr_size;
}

struct mpu_rasr_attributes {
  bool execute_never;
  uint8_t access;
  uint8_t tex;
  bool sharable;
  bool cachable;
  bool buffered;
};

const struct mpu_rasr_attributes rasr_attributes_code = {
  .execute_never = false,
  .access = 0x2, 
  .tex = 0x0,
  .sharable = true,
  .cachable = true,
  .buffered = true,
};

const struct mpu_rasr_attributes rasr_attributes_data = {
  .execute_never = true,
  .access = 0x3, 
  .tex = 0x1,
  .sharable = true,
  .cachable = true,
  .buffered = true,
};

const struct mpu_rasr_attributes rasr_attributes_peripherals = {
  .execute_never = true,
  .access = 0x3, 
  .tex = 0x0,
  .sharable = false,
  .cachable = false,
  .buffered = true,
};

static uint32_t mpu_rasr_attributes_bits(const struct mpu_rasr_attributes fields) {
    uint32_t result =
      ((uint32_t)fields.execute_never << 28) |
      ((uint32_t)fields.access << 24) |
      ((uint32_t)fields.tex << 19) |
      ((uint32_t)fields.sharable << 18) |
      ((uint32_t)fields.cachable << 17) |
      ((uint32_t)fields.buffered << 16);
    return result;
}

static uint32_t mpu_rasr(const struct region *r) {
  uint32_t result = 0;
  if (r->type == CUSTOM) {
    result = r->_custom;
  } else {
    switch (r->type) {
      case CODE_REGION:
        result = mpu_rasr_attributes_bits(rasr_attributes_code);
        break;
      case DATA_REGION:
      case STACK_REGION:
      case UNCACHED_DATA_REGION:
        result = mpu_rasr_attributes_bits(rasr_attributes_data);
        break;
      case PERIPHERAL_REGION:
        result = mpu_rasr_attributes_bits(rasr_attributes_peripherals);
        break;
      }

  }

  result |= (mpu_rasr_size(r->size) << 1);
  result |= 1; /* enable */

  return result;
}
  
void fiber_md_start() {
  uak_fiber_reschedule();
  volatile uint32_t *MPU_TYPE = (volatile uint32_t *)0x0E000ED90;
  volatile uint32_t *MPU_CTRL = (volatile uint32_t *)0x0E000ED94;
  volatile uint32_t *MPU_RBAR = (volatile uint32_t *)0x0E000ED9C;
  volatile uint32_t *MPU_RASR = (volatile uint32_t *)0x0E000EDA0;
  
  /* Enable memmanage and busfault */
  *((volatile uint32_t *)0xe000ed24) |= (1 << 18) |
                                        (1 << 17) |
                                        (1 << 16);

  *MPU_RBAR = executor.current->_mpu_context.regions[0].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[0].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[1].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[1].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[2].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[2].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[3].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[3].rasr;

  *MPU_RBAR = executor.current->_mpu_context.regions[4].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[4].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[5].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[5].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[6].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[6].rasr;
  *MPU_RBAR = executor.current->_mpu_context.regions[7].rbar;
  *MPU_RASR = executor.current->_mpu_context.regions[7].rasr;

  *MPU_CTRL = 5;
  
  syscall0(0xff);
}

extern struct executor executor;

__attribute__((used))
static void *uak_md_switch_context(void *old) {
  executor.current->_md = old;
  uak_fiber_reschedule();
  return executor.current->_md;
}

__attribute__((used))
static void *uak_md_get_mpu_ptrs(void) {
  return &executor.current->_mpu_context;
}

/* Pend SV is used to trigger a task change. This is totally written in assembly
 * because it is important that before the context is saved or after it is
 * loaded that we do not mangle any registers outside of r0-r3, since those are
 * the ones the hardware will handle. 
 * */
__asm__ (
        ".global PendSV_Handler\n"
         ".thumb_func\n"
        "PendSV_Handler:\n"

        /* Save the registers that aren't hardware-saved onto the stack still
         * pointed at by PSP */
        "mrs r0, psp\n"
        "tst lr, #0x10\n" /* Check for FP usage */
        "it eq\n"
        "vstmdbeq r0!, {S16-S31}\n"
        "stmdb r0!, {r4-r11, lr}\n"

        /* Actually switch current fiber */
        "bl uak_md_switch_context\n" 
        "mov r4, r0\n"

        /* Load r0 with current mpu context */
        "bl uak_md_get_mpu_ptrs\n" 
        "mov r1, r4\n"

        /* Load 8 MPU regions using the RBAR/RASR alias registers */
        "ldr r2, =0xe000ed9c\n" 
        "ldmia r0!, {r4-r11}\n"
        "stmia r2, {r4-r11}\n"
        "ldmia r0!, {r4-r11}\n"
        "stmia r2, {r4-r11}\n"

        /* r1 contains new context stack pointer */
        "ldmia r1!, {r4-r11, lr}\n"
        "tst lr, #0x10\n" /* Check for FP usage */
        "it eq\n"
        "vldmiaeq r1!, {S16-S31}\n"

        "msr psp, r1\n"
        /* We jump to the same LR that was saved to preserve FP state, otherwise
         * hardware will restore frame very wrong */
        "bx lr\n"
);

uint64_t cycle_count(void);

void SVC_Handler(uint32_t syscall, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
  switch (syscall) {
    case SYSCALL_NOTIFY_SET: {
      int32_t fiber_id = (int32_t)arg1;;
      uint32_t value = arg2;
 //     emit_trace(NOTIFY_SET, fiber_id);
      uak_notify_set_from_privileged(fiber_id, value);
      return;
            }
    case SYSCALL_NOTIFY_WAIT: {
      uint32_t result;
//      emit_trace(NOTIFY_WAIT, 0);
      if (uak_internal_notify_wait(&result)) {
        /* This fiber has already received a notification, return the value
         * immediately */
        __asm__(
            "mrs r0, psp\n"
            "str.w %0, [r0]\n"
            : : "r"(result) : "r0");
      return;
      }
      /* unreachable */
            break;
            }
    case SYSCALL_QUEUE_PUT: {
      int32_t queue_id = (int32_t)arg1;;
      const char *msg = (const char *)arg2;
//      emit_trace(NOTIFY_SET, fiber_id);
      uak_queue_put_from_privileged(queue_id, msg);
      return;
                            }
    case SYSCALL_QUEUE_GET: {
      int32_t queue_id = (int32_t)arg1;;
      char *msg = (char *)arg2;
//      emit_trace(NOTIFY_WAIT, 0);
      uak_internal_queue_get(queue_id, msg);
      break;
                            }
    case SYSCALL_GET_CYCLE_COUNT: {
             uint32_t v = cycle_count();
             __asm__(
                 "mrs r0, psp\n"
                 "str.w %0, [r0]\n"
                 : : "r"(v) : "r0");
              }
              break;
    case 0xff: { /* Start scheduler */
      __asm__(
          /* Set CONTROL.SPSEL to use PSP in thread mode */
          "mrs r0, control\n" 
          "orr r0, r0, #3\n" /* unprivileged, use PSP */
          "bic r0, r0, #4\n" /* Clear FP active */
          "msr control, r0\n"
          "mov r0, %0\n"
          "ldmia r0!, {r4-r11, lr}\n"
          "msr psp, r0\n"
          "bx lr\n"
          : : "r"(executor.current->_md));
               }
               break;

    default:
      break;
  }
}

#define MMFSR_MMAR_VALID (1<<7)
#define MMFSR_MSTKERR (1<<4)
#define MMFSR_MUNSTKERR (1<<3)
#define MMFSR_DACCVIOL (1<<1)
#define MMFSR_IACCVIOL (1<<0)
void MemManage_Handler(void) {
  volatile uint8_t *mmfsr_reg = (volatile uint8_t *)0xe000ed28;
  volatile uint32_t *mmfar_reg = (volatile uint32_t *)0xe000ed34;

  itm_debug("MemManage: ");
  uint8_t  mmfsr = *mmfsr_reg;
  if (mmfsr & MMFSR_MMAR_VALID) {
    itm_debug("MMAR_VALID ");
  }
  if (mmfsr & MMFSR_MSTKERR) {
    itm_debug("MSTKERR ");
  }
  if (mmfsr & MMFSR_MUNSTKERR) {
    itm_debug("MUNSTKERR ");
  }
  if (mmfsr & MMFSR_DACCVIOL) {
    itm_debug("DACCVIOL ");
  }
  if (mmfsr & MMFSR_IACCVIOL) {
    itm_debug("IACCVIOL ");
  }
  itm_debug("\n");
  char buf[32];
  if (mmfsr & MMFSR_MMAR_VALID) {
    sprintf(buf, "  MMAR: %x\n", *mmfar_reg);
    itm_debug(buf);
  }
  uint32_t *psp;
  __asm__("mrs %0, psp\n" : "=r"(psp));
  sprintf(buf, "  PC: %x\n", psp[6]);
  itm_debug(buf);

  while (true) {
  }
}



static uint32_t uak_md_lock_scheduler() {
  uint32_t result;
  __asm__("mrs %0, PRIMASK\n" :
          "=r"(result) : );
  __asm__("cpsid i" : : : "memory");
 return result;
}

static void uak_md_unlock_scheduler(uint32_t primask) {
  __asm__("msr PRIMASK, %0" : : "r"(primask) : "memory");
}

static int32_t uak_md_fiber_add_region(struct fiber *f, const struct region *r) {

  /* Ensure region size is power of 2 greater or equal to 32 */
  if (r->size < 32) {
    return -1;
  }
  if ((r->size & (r->size - 1)) != 0) {
    return -1;
  }
  /* Ensure region is aligned by size */
  if ((r->start & (r->size - 1)) != 0) {
    return -1;
  }

  /* Find a usable region */
  int32_t region_idx = -1;
  for (int i = 0; i < 8; i++) {
    if (f->_mpu_context.regions[i].rasr == 0) {
      region_idx = i;
      break;
    }
  }
  if (region_idx < 0) {
    return -1;
  }
  /* region_idx is an unused region */

  f->_mpu_context.regions[region_idx] = (struct mpu_region){
    .rbar = r->start | (1 << 4) | region_idx,
    .rasr = mpu_rasr(r),
  };
  return region_idx;
}

