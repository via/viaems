AS=arm-none-eabi-as
CC=arm-none-eabi-gcc
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

LIBUSB=$(PWD)/contrib/libusb_stm32
LIBUSB_LIB=libusbd.a
CMSIS=/home/via/dev/viaems/contrib/CMSIS_5/
CMSISDEV=/home/via/dev/viaems/contrib/CMSIS_Device/



OBJS+= stm32h743_init.o \
       stm32h743_sched.o \
       stm32h743_sensors.o \
       stm32h743_vectors.o \
       stm32h743_usb.o \
       stm32h743_flash.o \
       stm32h743_pwm.o \
       ${LIBUSB_LIB}


OBJS+= libssp.a libssp_nonshared.a

ALL_CFLAGS= -DNDEBUG -ffunction-sections -fdata-sections -O0 -ggdb \
            -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -mcpu=cortex-m7
CFLAGS+= -I${CMSIS}/CMSIS/Core/Include
CFLAGS+= -I${CMSISDEV}/ST/STM32H7xx/Include
CFLAGS+= -I${LIBUSB}/inc
CFLAGS+= -D TICKRATE=4000000
CFLAGS+= -D STM32H7 -D STM32H743xx
CFLAGS+= ${ALL_CFLAGS}

LDFLAGS+= -lc -lnosys -nostartfiles -L ${OBJDIR} -Wl,--gc-sections --specs=nano.specs
LDFLAGS+= -T src/platforms/stm32h743/stm32h743.ld

${OBJDIR}/${LIBUSB_LIB}:
	-rm ${LIBUSB}/${LIBUSB_LIB}
	$(MAKE) -C ${LIBUSB} \
		MODULE="${LIBUSB_LIB}" \
		CFLAGS="${ALL_CFLAGS}" \
		CMSIS=${CMSIS} \
		CMSISDEV=${CMSISDEV} \
		DEFINES="STM32H7 STM32H743xx USBD_SOF_DISABLED" \
		clean module
	cp ${LIBUSB}/${LIBUSB_LIB} ${OBJDIR}/${LIBUSB_LIB}

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

${OBJDIR}/viaems.dfu: ${OBJDIR}/viaems
	${OBJCOPY} -O binary ${OBJDIR}/viaems ${OBJDIR}/viaems.dfu

program: ${OBJDIR}/viaems.dfu
	dfu-util -D ${OBJDIR}/viaems.dfu -a 0 -s 0x8000000:leave
