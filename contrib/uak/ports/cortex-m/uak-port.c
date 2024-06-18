extern struct executor executor;

__attribute__((used))
static void *uak_md_switch_context(void *old) {
  int prior = executor.current - executor.fibers;
  executor.current->_md = old;
  uak_fiber_reschedule();
  int new = executor.current - executor.fibers;
  emit_trace(FIBER_SWITCH, prior, new, 0);
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

void SVC_Handler() {
  if (!executor.started) {
    executor.started = true;
    __asm__(
        /* Set CONTROL.SPSEL to use PSP in thread mode */
        "mrs r0, control\n" 
        "orr r0, r0, #3\n" /* unprivileged, use PSP */
        "bic r0, r0, #4\n" /* Clear FP active */
        "msr control, r0\n"
        "mov r0, %0\n"
        "ldmia r0!, {r4-r11, lr}\n"
        "msr psp, r0\n"
        "cpsie i\n"
        "bx lr\n"
        : : "r"(executor.current->_md));
  }

  uint8_t this_fiber_id = executor.current - executor.fibers;
  uint32_t *PSP;
  /* process stack */
  __asm__("mrs %0, PSP" : "=r"(PSP));
  uint32_t syscall = PSP[0];
  uint32_t arg1 = PSP[1];
  uint32_t arg2 = PSP[2];
  uint32_t arg3 = PSP[3];
  switch (syscall) {
    case SYSCALL_NOTIFY_SET: {
      int32_t fiber_id = (int32_t)arg1;;
      uint32_t value = arg2;
      emit_trace(NOTIFY_SET, this_fiber_id, fiber_id, 0);
      uak_notify_set_from_privileged(fiber_id, value);
      return;
            }
    case SYSCALL_NOTIFY_WAIT: {
      uint32_t result;
      emit_trace(NOTIFY_WAIT, this_fiber_id, 0, 0);
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
      emit_trace(QUEUE_PUT, this_fiber_id, queue_id, 0);
      uak_queue_put_from_privileged(queue_id, msg);
      return;
                            }
    case SYSCALL_QUEUE_GET: {
      int32_t queue_id = (int32_t)arg1;;
      char *msg = (char *)arg2;
      emit_trace(QUEUE_GET, this_fiber_id, queue_id, 0);
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
    default:
      break;
  }
}

void itm_debug(const char *);
 __attribute__((weak)) int32_t uak_initialization_failure(const char *msg) {
   itm_debug("initialization failure: ");
   itm_debug(msg);
   itm_debug("\n");
   while (1);
}

 __attribute__((weak)) int32_t uak_runtime_failure(const char *msg) {
   itm_debug("runtime failure: ");
   itm_debug(msg);
   itm_debug("\n");
   while (1);
}


/* Port-specific runtime initialization of fiber. For cortex-m we initialize the
 * stack for a return from interrupt to user code */
void uak_port_initialize_fiber(struct fiber *fiber) {

  uint32_t *stack_pointer = fiber->stack[f->stack_size / 4];
  if ((uint32_t)stack_pointer & 0x4) {
    /* Ensure we are aligned to 8 bytes for the top of the stack */
    stack_pointer -= 1;
  }

  *(stack_pointer - 9) = 0xfffffffd;
  *(stack_pointer - 8) = 0; /* r0 */
  *(stack_pointer - 7) = 1; /* r1 */
  *(stack_pointer - 6) = 2; /* r2 */
  *(stack_pointer - 5) = 3; /* r3 */
  *(stack_pointer - 4) = 12; /* r12 */
  *(stack_pointer - 3) = (uint32_t)hang_forever; /* lr */
  *(stack_pointer - 2) = (uint32_t)f->entry; /* pc */
  *(stack_pointer - 1) = 0x21000000; /* psr */
  f->context.context = stack_pointer - 17;
}


