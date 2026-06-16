TARGET  = blink
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CPUFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

CFLAGS   = $(CPUFLAGS) -O0 -g -Wall
CFLAGS  += -DSTM32F446xx
CFLAGS  += -Iinclude

LDFLAGS  = -T stm32f446re.ld
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--no-warn-rwx-segments

SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET).bin
	@arm-none-eabi-size $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: startup_stm32f446xx.s $(OBJS)
	$(CC) $(CPUFLAGS) $(LDFLAGS) $^ -o $@ -lgcc

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	openocd \
		-f interface/stlink.cfg \
		-f target/stm32f4x.cfg \
		-c "program $(TARGET).bin verify reset exit 0x08000000"

clean:
	rm -f *.o *.elf *.bin

.PHONY: all flash clean