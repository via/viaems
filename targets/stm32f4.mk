CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
SPI_ADC?=TLV2553

CMSIS=contrib/CMSIS_5/
CMSISDEV=contrib/cmsis_device_f4/
LIBUSB=contrib/libusb_stm32

VPATH=src/platforms/${PLATFORM} \
      contrib/libusb_stm32/src \
			contrib/uak/ \
      contrib/uak/md

LIBUSB_OBJS= usbd_core.o \
             usbd_stm32f429_otgfs.o


UAK_OBJS = fiber.o 
#					 cortex-m4f-m7.o

OBJS+= stm32f4.o \
       stm32f4_sched.o \
       stm32f4_vectors.o \
       stm32f4_usb.o \
       stm32f4_sensors.o \
       stm32f4_pwm.o \
       stm32_sched_buffers.o \
			 ${UAK_OBJS} \
       ${LIBUSB_OBJS}


OBJS+= libssp.a libssp_nonshared.a

ASFLAGS= -g -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -mfloat-abi=hard

CFLAGS= -DNDEBUG -ffunction-sections -fdata-sections -O3 -g3
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4 -pipe
CFLAGS+= -DSTM32F4 -DSTM32F4xx -DSTM32F427xx
CFLAGS+= -DPLATFORMIO -DUSBD_SOF_DISABLED

CFLAGS+= -I${CMSIS}/CMSIS/Core/Include
CFLAGS+= -I${CMSISDEV}/Include
CFLAGS+= -I${LIBUSB}/inc
CFLAGS+= -DTICKRATE=4000000 -DSPI_${SPI_ADC}
CFLAGS+= -Icontrib/uak
CFLAGS+= -Isrc/platforms/stm32f4

LDFLAGS+= -lc -lnosys -L ${OBJDIR} -nostartfiles -Wl,--gc-sections
LDFLAGS+= -T src/platforms/stm32f4/stm32f4.ld

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

${OBJDIR}/viaems.dfu: ${OBJDIR}/viaems
	${OBJCOPY} -O ihex --remove-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems.hex
	${OBJCOPY} -O ihex --only-section=.configdata ${OBJDIR}/viaems ${OBJDIR}/viaems-config.hex
	scripts/dfuse-pack.py -i ${OBJDIR}/viaems.hex -i ${OBJDIR}/viaems-config.hex ${OBJDIR}/viaems.dfu

program: ${OBJDIR}/viaems.dfu
	dfu-util -D ${OBJDIR}/viaems.dfu -s :leave
