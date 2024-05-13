#include <stdint.h>

#include "fiber.h"
#include "fiber-private.h"

/* Uak implementation for cortex-m4/7.
 *
 * MPU regions:
 * 0 - fiber(s) code region 0x08000000 - _etext(08002110) (16K)
 * 1 - fiber(s) data + bss 0x20000000 - _ebss(20009398) (64K)
 * 2 - fiber stack
 */

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

static uint32_t mpu_asr(bool xn, 
    uint8_t ap, 
    uint8_t tex, 
    bool s,
    bool c,
    bool b,
    uint8_t srd,
    uint8_t size
    ) {
  uint32_t result =
    ((uint32_t)xn << 28) |
    ((uint32_t)ap << 24) |
    ((uint32_t)tex << 19) |
    ((uint32_t)s << 18) |
    ((uint32_t)c << 17) |
    ((uint32_t)b << 16) |
    ((uint32_t)srd << 8) |
    ((uint32_t)srd << 8) |
    ((uint32_t)size << 1) | 1;
  return result;
}

extern uint32_t _stext_test_loops;
extern uint32_t _etext_test_loops;

extern uint32_t _sdata_test_loops;
extern uint32_t _edata_test_loops;

extern uint32_t t1_stack[128];
extern uint32_t t2_stack[128];

  
void fiber_md_start() {
  uak_fiber_reschedule();
  volatile uint32_t *MPU_TYPE = (volatile uint32_t *)0x0E000ED90;
  volatile uint32_t *MPU_CTRL = (volatile uint32_t *)0x0E000ED94;
  volatile uint32_t *MPU_RBAR = (volatile uint32_t *)0x0E000ED9C;
  volatile uint32_t *MPU_RASR = (volatile uint32_t *)0x0E000EDA0;
  
#define VALID (1<<4)
  /* Enable memmanage and busfault */
  *((volatile uint32_t *)0xe000ed24) |= (1 << 18) |
                                        (1 << 17) |
                                        (1 << 16);
  // size 15 (64k)
  *MPU_RBAR = (uint32_t)&_stext_test_loops | VALID | 0;
  *MPU_RASR = mpu_asr(false, 2, 0, true, true, true,
      0, 11);

  // size 11 (4k)
  *MPU_RBAR = (uint32_t)&_sdata_test_loops | VALID | 1;
  *MPU_RASR = mpu_asr(true, 3, 1, true, true, true,
      0, 11);

  // size 8 (512b)
  *MPU_RBAR = (uint32_t)t1_stack | VALID | 2;
  *MPU_RASR = mpu_asr(true, 3, 1, true, true, true,
      0, 8);
  //
  // size 8 (512b)
  *MPU_RBAR = (uint32_t)t2_stack | VALID | 3;
  *MPU_RASR = mpu_asr(true, 3, 1, true, true, true,
      0, 8);

  *MPU_RBAR = VALID | 4;
  *MPU_RASR = 0;
  *MPU_RBAR = VALID | 5;
  *MPU_RASR = 0;
  *MPU_RBAR = VALID | 6;
  *MPU_RASR = 0;
  *MPU_RBAR = VALID | 7;
  *MPU_RASR = 0;

  *MPU_CTRL = 5;
  
  syscall0(0xff);
#if 0
  __asm__(
      "mov r0, %0\n"
      "ldmia r0!, {r4-r11, lr}\n"
      "msr psp, r0\n"

      /* Set CONTROL.SPSEL to use PSP in thread mode */
      "mrs r0, control\n" 
      "orr r0, r0, #3\n" /* unprivileged, use PSP */
      "bic r0, r0, #4\n" /* Clear FP active */
      "msr control, r0\n"
      "isb\n"


      "ldmia sp!, {r0-r3, r12, lr}\n"
      "pop {pc}\n" 
      "nop\n"
      : : "r" (executor.current->_md));
#endif
}

extern struct executor executor;

__attribute__((used))
static void *platform_fiber_switch_stack(void *old) {
  executor.current->_md = old;
  uak_fiber_reschedule();
  return executor.current->_md;
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
        "bl platform_fiber_switch_stack\n" 

        "ldmia r0!, {r4-r11, lr}\n"
        "tst lr, #0x10\n" /* Check for FP usage */
        "it eq\n"
        "vldmiaeq r0!, {S16-S31}\n"

        "msr psp, r0\n"
        /* We jump to the same LR that was saved to preserve FP state, otherwise
         * hardware will restore frame very wrong */
        "bx lr\n"
);

uint64_t cycle_count(void);

void SVC_Handler(uint32_t syscall, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
  switch (syscall) {
    case 1: {
      int32_t fiber_id = (int32_t)arg1;;
      uint32_t value = arg2;
      uak_notify_set_from_privileged(fiber_id, value);
      return;
            }
    case 2: {
      uint32_t result;
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
    case 0xf: {
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

void MemManage_Handler(void) {
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

