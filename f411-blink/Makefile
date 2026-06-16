TARGET  = blink
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CPUFLAGS  = -mcpu=cortex-m4
CPUFLAGS += -mthumb
CPUFLAGS += -mfloat-abi=hard
CPUFLAGS += -mfpu=fpv4-sp-d16

CFLAGS   = $(CPUFLAGS)
CFLAGS  += -O0
CFLAGS  += -g
CFLAGS  += -Wall
CFLAGS  += -DSTM32F411xE
CFLAGS  += -Iinclude

LDFLAGS  = -T stm32f411xe.ld
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--no-warn-rwx-segments

SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET).bin
	@arm-none-eabi-size $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: startup_stm32f411xe.s $(OBJS)
	$(CC) $(CPUFLAGS) $(LDFLAGS) $^ -o $@ -lgcc

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

clean:
	rm -f *.o *.elf *.bin

.PHONY: all flash clean