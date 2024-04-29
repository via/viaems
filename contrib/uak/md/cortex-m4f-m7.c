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
  *(sp - 7) = 0; /* r1 */
  *(sp - 6) = 0; /* r2 */
  *(sp - 5) = 0; /* r3 */
  *(sp - 4) = 0; /* r12 */
  *(sp - 3) = (uint32_t)hang_forever; /* lr */
  *(sp - 2) = (uint32_t)f->entry; /* pc */
  *(sp - 1) = 0x21000000; /* psr */
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
        "cpsid i\n"

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
        "cpsie i\n"
        "bx lr\n"
);

void SVC_Handler(void) {
  __asm__(
        "mov r0, %0\n"
        /* Restore non-hardware-saved context */
        "ldmia r0!, {r4-r11, lr}\n"
        "msr psp, r0\n"
        "bx lr\n"
      : :  "r" (executor.current->_md));

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

