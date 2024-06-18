
#define NUM_FIBERS 5

void console_loop(void *);
uint32_t console_stack[128] __attribute__((aligned(512)));

struct executor executor = {
  .fibers = {
    /* Process: console, fiber: console */
    { .state = UAK_RUNNABLE, .priority = 3, entry = console_loop,
      .argument = NULL, stack = console_stack, stack_size = 512,
      .context = { .regions = {
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
        { .rbar = 0x0, .rasr = 0x0, },
      }},
    },

  },
};

static void uak_initialize_executor(void) {
  /* Populate stacks */
  uak_port_initialize_fiber(&executor.fibers[0]);

  /* Make all fibers runnable (populate runqueues) */
  uak_make_runnable(&executor.fibers[0]);
}

