CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

CM3_LIB=libopencm3_stm32f4.a

OBJS+= ${CM3_LIB}
OBJS+= stm32f4-discovery.o
OBJS+= libssp.a libssp_nonshared.a

CFLAGS+= -D TICKRATE=4000000
CFLAGS+= -I${OPENCM3_DIR}/include -DSTM32F4
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4

LDFLAGS+= -lopencm3_stm32f4 -lc -lm -lnosys
LDFLAGS+= -T src/platforms/stm32f4-discovery.ld -nostartfiles
LDFLAGS+= -L arch/stm32f4 -l:${CM3_LIB}

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a


${OBJDIR}/${CM3_LIB}:
	$(MAKE) -C ${OPENCM3_DIR} ${MAKEFLAGS} clean
	$(MAKE) -C ${OPENCM3_DIR} ${MAKEFLAGS}
	cp ${OPENCM3_DIR}/lib/${CM3_LIB} ${OBJDIR}

