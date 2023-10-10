CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
SPI_ADC?=TLV2553

CMSIS=contrib/CMSIS_5/
CMSISDEV=contrib/CMSIS_Device_S32K344/

VPATH=src/platforms/${PLATFORM}

LIBUSB_OBJS= usbd_core.o \
             usbd_stm32f429_otgfs.o

FR_OBJS+= list.o \
          queue.o \
          stream_buffer.o \
          tasks.o \
          port.o

OBJS+= s32k344_boot.o


OBJS+= libssp.a libssp_nonshared.a

CFLAGS=  -DNDEBUG -ffunction-sections -fdata-sections -O3 -ggdb
CFLAGS+= -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -mcpu=cortex-m7
CFLAGS+= -DTICKRATE=4000000 -DSPI_${SPI_ADC}

CFLAGS+= -I${CMSIS}/CMSIS/Core/Include
CFLAGS+= -I${CMSISDEV}/Include
CFLAGS+= -Icontrib/FreeRTOS-Kernel/include
CFLAGS+= -Icontrib/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
CFLAGS+= -Isrc/platforms/s32k344

LDFLAGS+= -lc -lnosys -nostartfiles -L ${OBJDIR} -Wl,--gc-sections --specs=nano.specs
LDFLAGS+= -T src/platforms/s32k344/s32k344.ld

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a
