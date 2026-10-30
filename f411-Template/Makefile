TARGET  = blink
BUILD   = build
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

LDFLAGS  = -T linker/stm32f411xe.ld
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--no-warn-rwx-segments

SRCS = src/main.c
OBJS = $(SRCS:src/%.c=$(BUILD)/%.o)

all: $(BUILD)/$(TARGET).bin
	@arm-none-eabi-size $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: src/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: startup/startup_stm32f411xe.s $(OBJS)
	$(CC) $(CPUFLAGS) $(LDFLAGS) $^ -o $@ -lgcc

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD)/$(TARGET).bin
	st-flash write $(BUILD)/$(TARGET).bin 0x08000000

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean