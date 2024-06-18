#include "gen.h"

static struct uak_config config = {
	.processes = {
		{ .name = "console", 
		  .tasks = {
			{ .name = "console", .fn = "console_loop", .stack_size = 512, .priority = 3},
		}},
		{ .name = "engine", 
		  .tasks = {
			{ .name = "sensors", .fn = "sensor_loop", .stack_size = 512, .priority = 1},
			{ .name = "decoder", .fn = "decoder_loop", .stack_size = 512, .priority = 1},
			{ .name = "engine", .fn = "engine_loop", .stack_size = 512, .priority = 1},
			{ .name = "sim", .fn = "sim_loop", .stack_size = 512, .priority = 1},
		}},
	},
	.queues = {
		{ .name = "triggers", .type = "struct trigger_event", .size = 1 },
		{ .name = "adc", .type = "struct adc_update", .size = 1 },
		{ .name = "engine_update", .type = "struct engine_pump_update", .size = 1 },
	},
	.syscalls = {
		{ .name = "get_cycle_count", .return_type = "uint32_t" },
		{ .name = "set_output", .args = {"uint32_t", "uint32_t", NULL }},
		{ .name = "set_pwm", .args = {"int", "float", NULL }},
	},
};

