CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

OPENCM3_DIR=$(PWD)/libopencm3
CM3_LIB=libopencm3_stm32f4.a

OBJS+= ${CM3_LIB}
OBJS+= stm32f4-discovery.o
OBJS+= libssp.a libssp_nonshared.a

CFLAGS+= -D TICKRATE=4000000
CFLAGS+= -I${OPENCM3_DIR}/include -DSTM32F4 -O3
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4

LDFLAGS+= -lc -lnosys -L ${OBJDIR} -l:${CM3_LIB} 
LDFLAGS+= -T src/platforms/stm32f4-discovery.ld -nostartfiles

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
