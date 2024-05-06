#include <stdint.h>

#include "fiber.h"
#include "fiber-private.h"

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

void fiber_md_start() {
  uak_fiber_reschedule();
  __asm__(
      "mov r0, %0\n"
      "ldmia r0!, {r4-r11, lr}\n"
      "msr psp, r0\n"

      /* Set CONTROL.SPSEL to use PSP in thread mode */
      "mrs r0, control\n" 
      "orr r0, r0, #2\n"
      "bic r0, r0, #4\n" /* Clear FP active */
      "msr control, r0\n"
      "isb\n"


      "ldmia sp!, {r0-r3, r12, lr}\n"
      "pop {pc}\n" 
      "nop\n"
      : : "r" (executor.current->_md));
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

void internal_uak_notify_set(int32_t fiber, uint32_t value);
int32_t internal_wait_on_notify(int32_t fiber);

int32_t SVC_Handler(uint32_t syscall, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
  switch (syscall) {
    case 1: {
      int32_t fiber_id = (int32_t)arg1;;
      uint32_t value = arg2;
      internal_uak_notify_set(fiber_id, value);
      return 0;
            }
    case 2: {
      int32_t fiber_id = (int32_t)arg1;;
      return internal_wait_on_notify(fiber_id);
            }
    default:
      break;
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

