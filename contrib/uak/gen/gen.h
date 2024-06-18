#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct uak_queue {
	const char *name;
	const char *type;
	size_t size;
};

struct uak_task {
	const char *name;
	const char *fn;
	size_t stack_size;
	int priority;

	const char **notify_wakeup_allowlist;
	const char **queue_get_allowlist;
	const char **queue_put_allowlist;
};

struct region { };

struct uak_process {
	const char *name;
	struct uak_task tasks[16];
	struct region shm_regions[4];
};

struct uak_custom_syscall {
	const char *name;
	const char *return_type;
	const char *args[8];
	const char *fn;
};


struct uak_config {
	struct uak_process processes[8];
	struct uak_queue queues[32];
	struct uak_custom_syscall syscalls[32];
};

