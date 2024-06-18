#include "uak_config.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char *upper(const char *src) {
	char *result = strdup(src);
	for (int i = 0; i < strlen(result); i++) {
		if (isalpha(result[i])) {
			result[i] = toupper(result[i]);
		}
	}
	return result;
}

static void generate_syscall_trampoline(FILE *f, int sysno, const struct uak_custom_syscall *s) {
	    char *uppername = upper(s->name);
	    int n_args = 0;
	    for (const char *const *arg = s->args; *arg; arg++, n_args++);

	    fprintf(f, "/* Syscall trampoline for %s */\n", s->name); 
	    fprintf(f, "#define UAK_SYSCALL_%s %d\n", uppername, sysno);
	    fprintf(f, "struct uak_syscall_params_%s {\n", s->name);
	    for (int i = 0; i < n_args; i++) {
		    fprintf(f, "  %s p%d;\n", s->args[i], i);
	    }
	    if (s->return_type) {
		    fprintf(f, "  %s return_value;\n", s->return_type);
	    }
	    fprintf(f, "};\n\n");


	    fprintf(f, "static inline %s uak_syscall_%s(", 
			    s->return_type ? s->return_type : "void", 
			    s->name);
	    for (int i = 0; i < n_args; i++){ 
		    fprintf(f, "%s p%d%s", 
				    s->args[i], 
				    i, 
				    (i == n_args - 1) ? "" : ", ");
	    }
	    fprintf(f, ") {\n");
	    fprintf(f, "  struct uak_syscall_params_%s params = {\n", s->name);
	    for (int i = 0; i < n_args; i++){ 
		    fprintf(f, "    .p%d = p%d,\n", i, i);
	    }
	    fprintf(f, "  };\n");
	    fprintf(f, "  uak_syscall(UAK_SYSCALL_%s, (void *)&params);\n", uppername);
	    if (s->return_type) {
		    fprintf(f, "  return params.return_value;\n");
	    }

	    fprintf(f, "}\n\n"); 
	    free(uppername);
}

static void generate_syscall_dispatcher_stanza(FILE *f, const struct uak_custom_syscall *s) {
	char *uppername = upper(s->name);
	int n_args = 0;
	for (const char *const *arg = s->args; *arg; arg++, n_args++);

	fprintf(f, "    case UAK_SYSCALL_%s: {\n", uppername);
	fprintf(f, "      struct uak_syscall_params_%s *params = arg;\n", s->name);
	fprintf(f, "      %s%s(", s->return_type ? "params->return_value = " : "",
			          s->fn ? s->fn : s->name);
	for (int i = 0; i < n_args; i++) {
		fprintf(f, "params->p%d%s", i, (i == n_args - 1) ? "" : ", ");
	}
	fprintf(f, ");\n");
	fprintf(f, "      break;\n");
	fprintf(f, "    }\n");
}

static void generate_executor_defines(FILE *f, struct uak_config *conf) {
  int n_fibers = 0;
  int n_fibers_per_priority[32] = { 0 };
  for (struct uak_process *p = conf->processes; p->name != NULL; p++) {
    for (struct uak_task *t = p->tasks; t->name != NULL; t++) {
      n_fibers++;
      n_fibers_per_priority[t->priority]++;
    }
  }

  fprintf(f, "#define NUM_FIBERS %d\n", 

}

int main(void) {

	struct uak_custom_syscall builtin_syscalls[] = {
	  { .name = "notify_set", .args = {"int32_t", "uint32_t"} },
	  { .name = "notify_get", .return_type = "uint32_t", .args = {"int32_t"}},
	  { .name = "queue_put", .args = {"int32_t", "const char *"}}, 
	  { .name = "queue_get", .args = {"int32_t", "char *"}},
	  { 0 },
	};

	int syscall_number = 0;
	for (struct uak_custom_syscall *s = builtin_syscalls; s->name; s++, syscall_number++) {
		generate_syscall_trampoline(stdout, syscall_number, s);
	}


	for (struct uak_custom_syscall *s = config.syscalls; s->name; s++, syscall_number++) {
		generate_syscall_trampoline(stdout, syscall_number, s);
	}

	fprintf(stdout, 
			"\n"
	                "static void uak_syscall_dispatcher(int sysno, void *arg) {\n"
			"  switch (sysno) {\n");
	for (struct uak_custom_syscall *s = builtin_syscalls; s->name; s++, syscall_number++) {
		generate_syscall_dispatcher_stanza(stdout, s);
	}
	for (struct uak_custom_syscall *s = config.syscalls; s->name; s++, syscall_number++) {
		generate_syscall_dispatcher_stanza(stdout, s);
	}
	fprintf(stdout, 
			"  }\n"
			"}\n");

}

