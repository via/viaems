/* Syscall trampoline for notify_set */
#define UAK_SYSCALL_NOTIFY_SET 0
struct uak_syscall_params_notify_set {
  int32_t p0;
  uint32_t p1;
};

static inline void uak_syscall_notify_set(int32_t p0, uint32_t p1) {
  struct uak_syscall_params_notify_set params = {
    .p0 = p0,
    .p1 = p1,
  };
  uak_syscall(UAK_SYSCALL_NOTIFY_SET, (void *)&params);
}

/* Syscall trampoline for notify_get */
#define UAK_SYSCALL_NOTIFY_GET 1
struct uak_syscall_params_notify_get {
  int32_t p0;
  uint32_t return_value;
};

static inline uint32_t uak_syscall_notify_get(int32_t p0) {
  struct uak_syscall_params_notify_get params = {
    .p0 = p0,
  };
  uak_syscall(UAK_SYSCALL_NOTIFY_GET, (void *)&params);
  return params.return_value;
}

/* Syscall trampoline for queue_put */
#define UAK_SYSCALL_QUEUE_PUT 2
struct uak_syscall_params_queue_put {
  int32_t p0;
  const char * p1;
};

static inline void uak_syscall_queue_put(int32_t p0, const char * p1) {
  struct uak_syscall_params_queue_put params = {
    .p0 = p0,
    .p1 = p1,
  };
  uak_syscall(UAK_SYSCALL_QUEUE_PUT, (void *)&params);
}

/* Syscall trampoline for queue_get */
#define UAK_SYSCALL_QUEUE_GET 3
struct uak_syscall_params_queue_get {
  int32_t p0;
  char * p1;
};

static inline void uak_syscall_queue_get(int32_t p0, char * p1) {
  struct uak_syscall_params_queue_get params = {
    .p0 = p0,
    .p1 = p1,
  };
  uak_syscall(UAK_SYSCALL_QUEUE_GET, (void *)&params);
}

/* Syscall trampoline for get_cycle_count */
#define UAK_SYSCALL_GET_CYCLE_COUNT 4
struct uak_syscall_params_get_cycle_count {
  uint32_t return_value;
};

static inline uint32_t uak_syscall_get_cycle_count() {
  struct uak_syscall_params_get_cycle_count params = {
  };
  uak_syscall(UAK_SYSCALL_GET_CYCLE_COUNT, (void *)&params);
  return params.return_value;
}

/* Syscall trampoline for set_output */
#define UAK_SYSCALL_SET_OUTPUT 5
struct uak_syscall_params_set_output {
  uint32_t p0;
  uint32_t p1;
};

static inline void uak_syscall_set_output(uint32_t p0, uint32_t p1) {
  struct uak_syscall_params_set_output params = {
    .p0 = p0,
    .p1 = p1,
  };
  uak_syscall(UAK_SYSCALL_SET_OUTPUT, (void *)&params);
}

/* Syscall trampoline for set_pwm */
#define UAK_SYSCALL_SET_PWM 6
struct uak_syscall_params_set_pwm {
  int p0;
  float p1;
};

static inline void uak_syscall_set_pwm(int p0, float p1) {
  struct uak_syscall_params_set_pwm params = {
    .p0 = p0,
    .p1 = p1,
  };
  uak_syscall(UAK_SYSCALL_SET_PWM, (void *)&params);
}


static void uak_syscall_dispatcher(int sysno, void *arg) {
  switch (sysno) {
    case UAK_SYSCALL_NOTIFY_SET: {
      struct uak_syscall_params_notify_set *params = arg;
      notify_set(params->p0, params->p1);
      break;
    }
    case UAK_SYSCALL_NOTIFY_GET: {
      struct uak_syscall_params_notify_get *params = arg;
      params->return_value = notify_get(params->p0);
      break;
    }
    case UAK_SYSCALL_QUEUE_PUT: {
      struct uak_syscall_params_queue_put *params = arg;
      queue_put(params->p0, params->p1);
      break;
    }
    case UAK_SYSCALL_QUEUE_GET: {
      struct uak_syscall_params_queue_get *params = arg;
      queue_get(params->p0, params->p1);
      break;
    }
    case UAK_SYSCALL_GET_CYCLE_COUNT: {
      struct uak_syscall_params_get_cycle_count *params = arg;
      params->return_value = get_cycle_count();
      break;
    }
    case UAK_SYSCALL_SET_OUTPUT: {
      struct uak_syscall_params_set_output *params = arg;
      set_output(params->p0, params->p1);
      break;
    }
    case UAK_SYSCALL_SET_PWM: {
      struct uak_syscall_params_set_pwm *params = arg;
      set_pwm(params->p0, params->p1);
      break;
    }
  }
}
