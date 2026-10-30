# Bare-Metal STM32 Development: Environment Setup Guide

**Platform:** Linux (Debian / Ubuntu)
**Worked example:** STM32F411 (Blackpill, `f411-blink`). F446 (Nucleo-64) substitutions are in the table in Step 9.

---

## Prerequisites

- A Linux PC with `sudo` access and internet
- An ST-Link probe (ST-Link/V2 dongle, or the on-board ST-Link of a Nucleo)
- An STM32F4 board: Blackpill (F411CEU6) or Nucleo-64 (F446RE / F411RE)
- Jumper wires for SWD, if using a separate probe: SWDIO, SWCLK, GND, 3V3
- Basic C and terminal skills

---

## Why Linux Over Windows

Bare-metal embedded development is command-line native. Toolchains, open-source RTOSes and most documentation assume a Unix environment. On Windows you fight PATH issues, Makefile incompatibilities and USB driver replacements. On Linux everything works natively.

---

## What You Need to Install

Four packages form the complete bare-metal toolchain:

| Tool              | Package             | Purpose                                                         |
|-------------------|---------------------|-----------------------------------------------------------------|
| arm-none-eabi-gcc | `gcc-arm-none-eabi` | Cross-compiler: compiles C for ARM Cortex-M4                    |
| make              | `make`              | Build system: automates compile and link steps                  |
| OpenOCD           | `openocd`           | Flash and debug bridge (also used by Cortex-Debug)              |
| st-info, st-flash | `stlink-tools`      | Probe and chip detection (`st-info`), simple flashing (`st-flash`) |

---

## Step 1: Update Your System

```bash
sudo apt update && sudo apt upgrade -y
```

---

## Step 2: Install the Toolchain

```bash
sudo apt install gcc-arm-none-eabi openocd stlink-tools make -y
```

---

## Step 3: Verify Installation

Every command must print a version number:

```bash
arm-none-eabi-gcc --version
openocd --version
make --version
st-info --version
```

Example output (versions vary by distribution):

```
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
...
Open On-Chip Debugger 0.12.0
...
GNU Make 4.4.1
...
v1.x.x
```

If any command returns "not found", re-run Step 2.

---

## Step 4: USB Permissions for the ST-Link

By default Linux blocks normal users from raw USB devices. Without the right udev rules, the tools fail with:

```
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

The `openocd` and `stlink-tools` packages already install udev rules for ST-Link probes, so you normally do **not** write your own. After installing:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then **unplug and replug the probe**.

Check the rules are present:

```bash
ls /etc/udev/rules.d /usr/lib/udev/rules.d | grep -i -E "stlink|openocd"
```

If you still get a permission error, add yourself to the `plugdev` group, then log out and back in:

```bash
sudo usermod -aG plugdev $USER
```

### Verify the Probe is Seen by Linux

```bash
lsusb | grep -i 0483
```

Vendor ID `0483` is STMicroelectronics. The product ID depends on the probe:

| Product ID | Probe                             |
|------------|-----------------------------------|
| `3748`     | ST-Link/V2 (external dongle)      |
| `374b`     | ST-Link/V2.1 (on-board, Nucleo)   |
| `374e`, `374f`, `3753` | ST-Link/V3            |

---

## Step 5: Check the Probe and the Chip

```bash
st-info --probe
```

Good output (F411 connected):

```
Found 1 stlink programmers
  version:    V2J46S7
  serial:     <probe serial>
  flash:      524288 (pagesize: 16384)
  sram:       131072
  chipid:     0x431
  dev-type:   STM32F411xC_xE
```

### Chip IDs

| `chipid` | Chip                      |
|----------|---------------------------|
| `0x431`  | STM32F411xC / xE          |
| `0x421`  | STM32F446xx               |
| `0x414`  | STM32F1 high-density (not an F4) |

`st-info --probe` must give the **same** chip ID every time for the same board.

### Troubleshooting `st-info --probe`

| Output                                  | Cause and fix                                                         |
|-----------------------------------------|-----------------------------------------------------------------------|
| `Found 0 stlink programmers`            | Probe not seen. Replug, try another cable or port, check `lsusb`      |
| `Failed to enter SWD mode`              | Probe found but no chip answering. Check SWDIO/SWCLK/GND/3V3, short wires, board powered |
| `flash: 0`, `chipid: 0x000`             | Same as above: no chip connected or loose wiring                      |
| Chip ID changes between runs            | Flaky wiring, or you swapped boards                                   |

---

## Step 6: Install VS Code

### Add the Microsoft Repository

```bash
sudo apt install wget gpg -y

wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg

sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/

sudo sh -c 'echo "deb [arch=amd64 signed-by=/etc/apt/trusted.gpg.d/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
```

### Install and Launch

```bash
sudo apt update
sudo apt install code -y
code
```

### Extensions

Press `Ctrl+Shift+X` and install:

| Extension    | Author    | Purpose                              |
|--------------|-----------|--------------------------------------|
| C/C++        | Microsoft | IntelliSense, syntax highlighting    |
| Cortex-Debug | marus25   | Connects the VS Code debugger to OpenOCD |

---

## Step 7: Create the Project Folder Structure

```bash
mkdir -p ~/stm32/f411-blink/{include/m-profile,linker,startup,src}
cd ~/stm32/f411-blink
echo "build/" > .gitignore
code .
```

`build/` is created automatically by `make`; do not create it by hand.

### Folder Purpose

| Path             | Contents                                                      |
|------------------|---------------------------------------------------------------|
| `include/`       | Header files: CMSIS core and ST device headers                |
| `include/m-profile/` | ARM Cortex-M profile headers (included by `cmsis_gcc.h`)  |
| `linker/`        | Linker script: maps code and data to Flash and RAM            |
| `startup/`       | Assembly startup file: vector table, runs before `main()`     |
| `src/`           | Your C source files                                           |
| `build/`         | Generated `.o`, `.elf`, `.bin`, never edited, git-ignored     |
| `Makefile`       | Build rules, in the project root                              |
| `.gitignore`     | Contains `build/`                                             |

---

## Step 8: Get CMSIS Core Headers From ARM

CMSIS (Cortex Microcontroller Software Interface Standard) core headers define the Cortex-M4 itself: SysTick, NVIC, core registers, intrinsics.

```bash
git clone --depth 1 https://github.com/ARM-software/CMSIS_6.git /tmp/cmsis

CORE=/tmp/cmsis/CMSIS/Core/Include
DEST=~/stm32/f411-blink/include

# Core headers
cp $CORE/core_cm4.h $CORE/cmsis_gcc.h $CORE/cmsis_compiler.h $CORE/cmsis_version.h $DEST/

# M-profile headers (all nine are needed; cmsis_gcc.h includes them)
cp $CORE/m-profile/armv7m_cachel1.h  $CORE/m-profile/armv7m_mpu.h \
   $CORE/m-profile/armv8m_mpu.h      $CORE/m-profile/armv8m_pmu.h \
   $CORE/m-profile/armv81m_pac.h     $CORE/m-profile/cmsis_armclang_m.h \
   $CORE/m-profile/cmsis_clang_m.h   $CORE/m-profile/cmsis_gcc_m.h \
   $CORE/m-profile/cmsis_iccarm_m.h  $DEST/m-profile/

rm -rf /tmp/cmsis
```

Keep `m-profile/` as a subfolder. The headers include each other by relative path.

---

## Step 9: Get ST Device Files

### Device headers and startup file

```bash
git clone --depth 1 https://github.com/STMicroelectronics/cmsis-device-f4.git /tmp/stm32f4

DEST=~/stm32/f411-blink

cp /tmp/stm32f4/Include/stm32f4xx.h        $DEST/include/
cp /tmp/stm32f4/Include/stm32f411xe.h      $DEST/include/
cp /tmp/stm32f4/Include/system_stm32f4xx.h $DEST/include/

cp /tmp/stm32f4/Source/Templates/gcc/startup_stm32f411xe.s $DEST/startup/

rm -rf /tmp/stm32f4
```

### Linker script from STM32CubeF4

The full STM32CubeF4 repository is very large. Fetch only the folder you need:

```bash
git clone --depth 1 --filter=blob:none --sparse https://github.com/STMicroelectronics/STM32CubeF4.git /tmp/cubef4
git -C /tmp/cubef4 sparse-checkout set Projects/STM32F411RE-Nucleo/Templates/STM32CubeIDE

cp /tmp/cubef4/Projects/STM32F411RE-Nucleo/Templates/STM32CubeIDE/STM32F411RETX_FLASH.ld \
   ~/stm32/f411-blink/linker/stm32f411xe.ld

rm -rf /tmp/cubef4
```

The F411RE script (512 KB Flash at `0x08000000`, 128 KB RAM at `0x20000000`) also matches the Blackpill's F411CEU6, which has the same memory sizes.

### Substitutions for the STM32F446RE (Nucleo-64)

| Item            | F411                                                           | F446                                                            |
|-----------------|----------------------------------------------------------------|-----------------------------------------------------------------|
| Device header   | `stm32f411xe.h`                                                | `stm32f446xx.h`                                                 |
| Startup file    | `startup_stm32f411xe.s`                                        | `startup_stm32f446xx.s`                                         |
| Linker (CubeF4 path) | `Projects/STM32F411RE-Nucleo/Templates/STM32CubeIDE/STM32F411RETX_FLASH.ld` | `Projects/STM32446E-Nucleo/Templates/STM32CubeIDE/STM32F446RETX_FLASH.ld` |
| Linker (local name) | `linker/stm32f411xe.ld`                                    | `linker/stm32f446re.ld`                                         |
| Compiler define | `-DSTM32F411xE`                                                | `-DSTM32F446xx`                                                 |

### What Each File Does

| File                    | Source        | Purpose                                              |
|-------------------------|---------------|------------------------------------------------------|
| `core_cm4.h`            | ARM CMSIS     | Cortex-M4 core: NVIC, SysTick, FPU                   |
| `cmsis_gcc.h`           | ARM CMSIS     | GCC compiler intrinsics                              |
| `cmsis_compiler.h`      | ARM CMSIS     | Compiler-agnostic wrapper                            |
| `cmsis_version.h`       | ARM CMSIS     | CMSIS version definitions                            |
| `m-profile/*.h`         | ARM CMSIS     | Cache, MPU, PMU, PAC and per-compiler helpers        |
| `stm32f4xx.h`           | ST GitHub     | Family header: includes the right chip header from the `-D` define |
| `stm32f411xe.h`         | ST GitHub     | Chip header: all peripheral registers and bit masks  |
| `system_stm32f4xx.h`    | ST GitHub     | `SystemInit()` and `SystemCoreClock` declarations    |
| `startup_stm32f411xe.s` | ST GitHub     | Vector table, `Reset_Handler`: copies `.data`, zeroes `.bss`, calls `main` |
| `stm32f411xe.ld`        | ST CubeF4     | Linker script: memory map and section placement      |
| `main.c`                | You write     | Your application                                     |

### Project Structure After Steps 7 to 9

```
f411-blink/
├── Makefile                       <- you write (Step 10)
├── .gitignore                     <- contains: build/
├── include/
│   ├── m-profile/
│   │   ├── armv7m_cachel1.h       <- ARM CMSIS
│   │   ├── armv7m_mpu.h           <- ARM CMSIS
│   │   ├── armv8m_mpu.h           <- ARM CMSIS
│   │   ├── armv8m_pmu.h           <- ARM CMSIS
│   │   ├── armv81m_pac.h          <- ARM CMSIS
│   │   ├── cmsis_armclang_m.h     <- ARM CMSIS
│   │   ├── cmsis_clang_m.h        <- ARM CMSIS
│   │   ├── cmsis_gcc_m.h          <- ARM CMSIS
│   │   └── cmsis_iccarm_m.h       <- ARM CMSIS
│   ├── cmsis_compiler.h           <- ARM CMSIS
│   ├── cmsis_gcc.h                <- ARM CMSIS
│   ├── cmsis_version.h            <- ARM CMSIS
│   ├── core_cm4.h                 <- ARM CMSIS
│   ├── stm32f4xx.h                <- ST GitHub
│   ├── stm32f411xe.h              <- ST GitHub
│   └── system_stm32f4xx.h         <- ST GitHub
├── linker/
│   └── stm32f411xe.ld             <- ST CubeF4
├── startup/
│   └── startup_stm32f411xe.s      <- ST GitHub
├── src/
│   └── main.c                     <- you write (Step 11)
└── build/                         <- generated by make
```

---

## Step 10: The Makefile

Save as `Makefile` in the project root. Recipe lines **must start with a Tab**, not spaces, otherwise `make` fails with `missing separator`.

```makefile
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

flash-openocd: $(BUILD)/$(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program $(BUILD)/$(TARGET).bin verify reset exit 0x08000000"

clean:
	rm -rf $(BUILD)

.PHONY: all flash flash-openocd clean
```

For the F446, change three things: `-DSTM32F446xx`, `linker/stm32f446re.ld`, `startup/startup_stm32f446xx.s`.

### Makefile Flag Reference

| Flag                             | Meaning                                                   |
|----------------------------------|-----------------------------------------------------------|
| `-mcpu=cortex-m4`                | Target Cortex-M4 CPU                                      |
| `-mthumb`                        | Thumb-2 instruction set (mandatory for Cortex-M)          |
| `-mfpu=fpv4-sp-d16`              | Use the single-precision hardware FPU                     |
| `-mfloat-abi=hard`               | Pass floats in FPU registers                              |
| `-DSTM32F411xE`                  | Tells `stm32f4xx.h` which chip header to include          |
| `-Iinclude`                      | Header search path                                        |
| `-O0 -g`                         | No optimisation, debug symbols (for Cortex-Debug)         |
| `-T linker/stm32f411xe.ld`       | Linker script                                             |
| `-nostdlib`                      | No standard C library (no OS)                             |
| `-Wl,--gc-sections`              | Strip unused sections from the final binary               |
| `-Wl,--no-warn-rwx-segments`     | Silence a newer-binutils warning about the segment layout |
| `-lgcc`                          | Compiler support routines needed even without libc        |

---

## Step 11: Blink Example

This is the bare-metal Hello World. It blinks an on-board LED using direct register access: no HAL, no driver library.

### Nucleo-64 (F446RE or F411RE): LD2 on PA5

Save as `src/main.c`:

```c
#include <stdint.h>

#define RCC_BASE        0x40023800UL
#define GPIOA_BASE      0x40020000UL

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE   + 0x30))
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14))

void SystemInit(void) {}   /* called by startup before main() */
void _init(void)      {}   /* may be referenced by libc init  */

static void delay(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    RCC_AHB1ENR |= (1U << 0);       /* enable GPIOA clock    */
    GPIOA_MODER &= ~(3U << 10);     /* clear PA5 mode bits   */
    GPIOA_MODER |=  (1U << 10);     /* PA5 = output          */

    while (1)
    {
        GPIOA_ODR |=  (1U << 5);    /* LED ON  */
        delay(160000);
        GPIOA_ODR &= ~(1U << 5);    /* LED OFF */
        delay(160000);
    }
}
```

### Blackpill (F411CEU6): LED on PC13, active low

| Change from the PA5 version | Value                                          |
|-----------------------------|------------------------------------------------|
| Port base                   | `GPIOC_BASE = 0x40020800UL`                    |
| Clock enable                | `RCC_AHB1ENR \|= (1U << 2)` (GPIOC)            |
| Mode bits                   | clear `3U << 26`, set `1U << 26` (pin 13 x 2)  |
| LED ON                      | clear ODR bit 13 (`&= ~(1U << 13)`)            |
| LED OFF                     | set ODR bit 13 (`\|= (1U << 13)`)              |

### Build

```bash
make
```

Expected output (paths and sizes will vary):

```
arm-none-eabi-gcc -mcpu=cortex-m4 ... -c src/main.c -o build/main.o
arm-none-eabi-gcc -mcpu=cortex-m4 ... -o build/blink.elf startup/startup_stm32f411xe.s build/main.o -lgcc
arm-none-eabi-objcopy -O binary build/blink.elf build/blink.bin
   text    data     bss     dec     hex filename
    ...
```

Errors at this stage are compile or link errors. Fix them before flashing.

### Flash

Connect the probe and check it first:

```bash
st-info --probe
make flash
```

`make flash` uses `st-flash`. To use OpenOCD instead:

```bash
make flash-openocd
```

With OpenOCD, the key lines to look for are:

| Line                  | Meaning                                              |
|-----------------------|------------------------------------------------------|
| `Target voltage: 3.x` | Board is powered and the probe is talking to it      |
| `Programming Finished`| All bytes written                                    |
| `Verified OK`         | Flash read back and matches `blink.bin`              |
| `Resetting Target`    | Board resets and your code starts running            |

### What You Should See

After the reset, the LED blinks fast (several times per second). Increase the `delay()` count to slow it down.

If it does not blink:
- Confirm the flash step reported success (`Verified OK` with OpenOCD)
- Confirm the LED pin for your board (PA5 on Nucleo-64, PC13 on Blackpill)
- Run `make clean`, then `make flash` again

---

## Command Cheat Sheet

Run all of these from the project root (`f411-blink/`):

```bash
make                  # build, print size
make flash            # build if needed, flash with st-flash
make flash-openocd    # same, with OpenOCD
make clean            # delete build/
make clean && make    # full rebuild
```

```bash
st-info --probe                              # probe and chip detection
st-flash reset                               # reset the board
st-flash erase                               # erase all of Flash
arm-none-eabi-size build/blink.elf           # Flash / RAM usage
arm-none-eabi-objdump -d build/blink.elf     # disassembly
arm-none-eabi-nm -n build/blink.elf          # symbol addresses, sorted
```

---

## Common Errors and Fixes

| Error                                           | Cause                                   | Fix                                                        |
|-------------------------------------------------|-----------------------------------------|------------------------------------------------------------|
| `libusb_open() failed ... ACCESS`               | udev rules not applied                  | Step 4: reload rules, replug, check `plugdev` group        |
| `Found 0 stlink programmers`                    | Probe not detected                      | Replug, other cable/port, check `lsusb`                    |
| `Failed to enter SWD mode`                      | No chip answering                       | Check SWD wiring, GND, 3V3, board power                    |
| `st-flash: command not found`                   | `stlink-tools` not installed            | `sudo apt install stlink-tools`                            |
| `Makefile: missing separator`                   | Spaces instead of Tab in a recipe line  | Re-indent recipe lines with a Tab                          |
| `fatal error: stm32f4xx.h: No such file`        | Header not in `include/`, or `-Iinclude` missing | Check Steps 8 and 9 and the Makefile             |
| `fatal error: m-profile/...h: No such file`     | Missing `m-profile` headers             | Copy all nine files in Step 8                              |
| `undefined reference to __libc_init_array`      | ST startup calls it; `-nostdlib` removes libc | Delete the `bl __libc_init_array` line in `Reset_Handler` in the startup file |
| `Verify Failed`                                 | Flash write error                       | `make clean`, rebuild, flash again                         |

---

## Quick Reference: All Commands in Order

```bash
# 1. Toolchain
sudo apt update && sudo apt upgrade -y
sudo apt install gcc-arm-none-eabi openocd stlink-tools make -y

# 2. Verify
arm-none-eabi-gcc --version
openocd --version
make --version
st-info --version

# 3. udev rules (shipped by the packages), then replug the probe
sudo udevadm control --reload-rules
sudo udevadm trigger

# 4. Probe and chip check
lsusb | grep -i 0483
st-info --probe

# 5. Project structure
mkdir -p ~/stm32/f411-blink/{include/m-profile,linker,startup,src}
echo "build/" > ~/stm32/f411-blink/.gitignore

# 6. CMSIS, ST headers, startup, linker: Steps 8 and 9

# 7. Write src/main.c and Makefile, then:
cd ~/stm32/f411-blink
make
make flash
```

---

## RAG Metadata

```
topic: STM32 bare-metal Linux setup, project structure, CMSIS, Makefile, flashing
chips: STM32F411 (Blackpill), STM32F446RE (Nucleo-64)
os: Debian / Ubuntu Linux
folders: include (not inc), linker, startup, src, build
keywords: arm-none-eabi-gcc, make, OpenOCD, stlink-tools, st-info, st-flash,
          udev, CMSIS, m-profile, linker script, startup file, Makefile,
          blink, project structure, ST-Link
```
