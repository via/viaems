CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
AR=arm-none-eabi-ar
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
SPI_ADC?=TLV2553

VPATH=src/platforms/${PLATFORM}

OBJS+= s32k144.o \
       s32k144_extra.o \
       s32k144_enet.o \
       s32k1xx_crc.o \
       s32k144_startup.o \
       s32k144_comms.o \
       mcxe246_hwtest.o



OBJS+= libssp.a libssp_nonshared.a

CFLAGS= -DNDEBUG -ffunction-sections -fdata-sections -O1 -g3
CFLAGS+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4

CFLAGS+= -DTICKRATE=40000000 -DSPI_${SPI_ADC}
CFLAGS+= -fstack-protector-all
#CFLAGS+= -DHWTEST_PINS
#CFLAGS+= -DPLATFORM_HAS_NATIVE_MESSAGE_IO

LDFLAGS+= -lc -lnosys -L ${OBJDIR} -nostartfiles -Wl,--gc-sections
# --specs=nano.specs
LDFLAGS+= -T src/platforms/s32k144/s32k144-lram.ld

${OBJDIR}/libssp.a:
	${AR} rcs ${OBJDIR}/libssp.a

${OBJDIR}/libssp_nonshared.a:
	${AR} rcs ${OBJDIR}/libssp_nonshared.a

