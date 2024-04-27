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
  *(sp - 8) = 0; /* r0 */
  *(sp - 7) = 0; /* r1 */
  *(sp - 6) = 0; /* r2 */
  *(sp - 5) = 0; /* r3 */
  *(sp - 4) = 0; /* r12 */
  *(sp - 3) = (uint32_t)hang_forever; /* lr */
  *(sp - 2) = (uint32_t)f->entry; /* pc */
  *(sp - 1) = 0x21000000; /* psr */
  /* Additionally, the fiber switch code expects ISR's LR to be at top of stack,
   * indicating if the FP unit was used (bit 4 is zero).  We default to a
   * non-FPU-used return to thread mode */
  *(sp - 17) = 0xfffffffd;
  f->_md = sp - 17;
}


void uak_md_request_reschedule() {
  volatile uint32_t *ICSR = (volatile uint32_t *)0x0E000ED04;
  uint32_t ICSR_PENDSVSET = (1ULL << 28);

  *ICSR = ICSR_PENDSVSET;
  __asm__("dsb");
  __asm__("isb");
}

void fiber_md_start() {
  uak_fiber_reschedule();
  __asm__(
      "svc 0\n"
      "nop\n"
      );
}

__attribute__((used))
static void *platform_fiber_switch_stack(void *old) {
  uak_current_fiber()->_md = old;
  uak_fiber_reschedule();
  return uak_current_fiber()->_md;
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
        "mrs r2, psp\n"
        "stmdb r2!, {r4-r11}\n"
        "mov r1, lr\n"

        /* See if this previous context used FP, and if so save it */
        "and.w r1, r1, 0x10\n"
        "cmp r1, 0x10\n"
        "beq.n after_fp_save\n"
        /* Cortex m4f autosaves s0-s15 */
        "vstmdb r2!, {S16-S31}\n"

        /* Lastly, save lr to the stack so we know if the context used FP */
        "after_fp_save:\n"
        "stmdb r2!, {lr}\n"
        "mov r0, r2\n"
        /* Actually switch current fiber */
        "bl platform_fiber_switch_stack\n" 
        /* New frame's LR was stored last, save it */
        "ldr r2, [r0]\n"
        /* After LR is the normal stack frame including FP */
        "adds r1, r0, 0x4\n"
        /* Check to see if we used FP in this new context */
        "and.w r3, r2, 0x10\n"
        "cmp r3, 0x10\n"
        /* If bit 4 is set, we did not use FP */
        "beq.n after_fp_restore\n"
        "vldmia r1!, {S16-S31}\n"

        "after_fp_restore:\n"
        /* Restore non-hardware-saved context */
        "ldmfd r1!, {r4-r11}\n"
        "msr psp, r1\n"
        /* We jump to the same LR that was saved to preserve FP state, otherwise
         * hardware will restore frame very wrong */
        "bx r2\n"
);

void SVC_Handler(void) {
  __asm__(
        "ldr r2, [%0]\n"
        /* After LR is the normal stack frame including FP */
        "adds r1, %0, 0x4\n"
        /* Restore non-hardware-saved context */
        "ldmfd r1!, {r4-r11}\n"
        "msr psp, r1\n"
        "bx r2\n"
      : :  "r" (uak_current_fiber()->_md));

}


static uint32_t primask = 0;
void uak_md_lock_scheduler() {
  __asm__("mrs %0, PRIMASK\n" :
          "=r"(primask) : );
  __asm__("cpsid i");
  __asm__("dsb");
  __asm__("isb");

}

void uak_md_unlock_scheduler() {
  __asm__("msr PRIMASK, %0" : : "r"(primask));
}

