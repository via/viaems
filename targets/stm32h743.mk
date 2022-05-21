CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

OBJS+= stm32h743_init.o \
       stm32h743_vectors.o \
       stm32h743_sched.o \
       stm32h743_sensors.o

OBJS+= libssp.a libssp_nonshared.a

CFLAGS+= -D TICKRATE=4000000 -DNDEBUG -ffunction-sections -fdata-sections
CFLAGS+= -Og -ggdb 
CFLAGS+= -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -mcpu=cortex-m7

LDFLAGS+= -lc -lnosys -L ${OBJDIR} -Wl,--gc-sections
LDFLAGS+= -T src/platforms/stm32h743/stm32h743.ld -nostartfiles

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

${OBJDIR}/${CM3_LIB}:
	$(MAKE) -C ${OPENCM3_DIR} ${MAKEFLAGS}
	cp ${OPENCM3_DIR}/lib/${CM3_LIB} ${OBJDIR}

${OBJDIR}/viaems.dfu: ${OBJDIR}/viaems
	${OBJCOPY} -O binary ${OBJDIR}/viaems ${OBJDIR}/viaems.dfu

program: ${OBJDIR}/viaems.dfu
	dfu-util -D ${OBJDIR}/viaems.dfu -a 0 -s 0x8000000:leave
