TARGET=main
all: burn

PREFIX=arm-none-eabi

CC=$(PREFIX)-gcc
LD=$(PREFIX)-gcc
AS=$(PREFIX)-as
CP=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump


OBJCOPYFLAGS = -O binary

BIN=$(CP) -O ihex 

DEFS = -DSTM32F042
#DEFS := $(DEFS) -DDEBUG

MCU = cortex-m0
MCFLAGS = -mcpu=$(MCU) 

# -mthumb -mlittle-endian -mthumb-interwork

STM32_INCLUDES = -Ilib -I. -ISTM32F0xx_StdPeriph_Driver/inc

OPTIMIZE  =  -O2 -Wall # -flto -ffunction-sections -fdata-sections  -Wl,--relax 

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE)  $(DEFS) \
	-I. \
	$(STM32_INCLUDES) \
	-I../embeddedcommon \
	 -Wl,-T,lib/flash.ld

CFLAGS+=-I.

AFLAGS	= $(MCFLAGS) 

SRC = main.c \
	systems.c \
	lib/system_stm32f0xx.c \
	usb.c \
	touch.c \
	adc.c \
	ws2812.c \
	STM32F0xx_StdPeriph_Driver/src/stm32f0xx_rcc.c \
	STM32F0xx_StdPeriph_Driver/src/stm32f0xx_adc.c \
	STM32F0xx_StdPeriph_Driver/src/stm32f0xx_gpio.c \
	STM32F0xx_StdPeriph_Driver/src/stm32f0xx_misc.c \
	lib/startup_stm32f042.s


burn : $(TARGET).bin
	openocd -f flash.cfg #-d3

terminal :
	openocd -f terminal.cfg

$(TARGET).bin : $(TARGET).out
	$(PREFIX)-size $^
	$(CP) $(OBJCOPYFLAGS) $< $@

$(TARGET).hex: $(EXECUTABLE)
	$(CP) -O ihex $^ $@

$(TARGET).out : $(SRC)
	$(CC) $(CFLAGS) $^ -lm -lc -lnosys  -o $@

clean:
	rm -f $(TARGET).lst $(TARGET).out $(TARGET).hex $(TARGET).bin $(TARGET).map  $(EXECUTABLE)
