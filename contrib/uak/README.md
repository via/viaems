


uak/
    fiber.c 
    fiber.h

    ports/
       cortex-m/
                uak-port.c - 
                uak-port-syscall.h - user-facing header: system call interface for userspace for builtin-functions
                uak-port-private.h - Internal header: e.g. mpu interfaces

                uak-gen-port.h - Header for the native gen utility to generate port-specific structures
    native/ - tool to generate a `gen` folder. Uses a
      gen.h
      gen.c

    generated/
        user/
             uak-port-custom-syscalls.h - Provides a set of userspace-facing custom system call entrypoints
        kernel/
             uak-port-custom-dispatcher.h - custom system call dispatcher
             uak-initializers.h - helper functions to set up stacks and data sections0
             uak-config.h - custom data structures (fibers, executor, memory regions, queues)
         uak-port-processes.ld - includable linker script to help
      
                
