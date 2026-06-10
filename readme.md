# Bare-Metal STM32 Development — Environment Setup Guide
## Platform: Linux (Ubuntu 22.04 / 24.04 LTS)

---

## Why Linux Over Windows

Bare-metal embedded development is command-line native. Every
professional toolchain, open-source RTOS, and documentation example
assumes a Unix environment. On Windows you fight the environment
constantly — PATH issues, Makefile incompatibilities, USB driver
replacements, and MinGW workarounds. On Linux everything works
natively out of the box.

---

## What You Need to Install

Three tools form the complete bare-metal toolchain:

| Tool               | Purpose                                              |
|--------------------|------------------------------------------------------|
| arm-none-eabi-gcc  | Cross-compiler — compiles C for ARM Cortex-M4        |
| OpenOCD            | Flash and debug bridge — talks to ST-LINK over USB   |
| make               | Build system — automates compile and link steps      |

---

## Step 1 — Update Your System

Always update before installing new packages:

```bash
sudo apt update && sudo apt upgrade -y
```

---

## Step 2 — Install the Toolchain

Install all three tools in one command:

```bash
sudo apt install gcc-arm-none-eabi openocd make -y
```

---

## Step 3 — Verify Installation

Run each verification command. Every one must print a version number:

```bash
arm-none-eabi-gcc --version
openocd --version
make --version
```

Expected output:

```
arm-none-eabi-gcc (15:13.2.rel1-2) 13.2.1 20231009
...

Open On-Chip Debugger 0.12.0
...

GNU Make 4.3
...
```

If any command returns "not found", re-run Step 2.

---

## Step 4 — Configure ST-LINK USB Permissions

By default Linux blocks normal users from accessing unknown USB
devices. Without this fix, flashing the Nucleo board produces:

```
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

The fix is a udev rule — a permanent permission configuration that
tells Linux to allow your user to access the ST-LINK USB device.

### What is a udev Rule

udev is the Linux device manager. Rules in /etc/udev/rules.d/ define
how Linux handles specific USB devices when they are plugged in. We
create one rule that matches the STM32 ST-LINK by its Vendor ID
(0483) and Product ID (374b) and grants access to normal users.

### Create the Rule

Run this single command — it writes the rule file directly:

```bash
echo 'ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev"' | sudo tee /etc/udev/rules.d/99-stlink.rules
```

### Apply the Rule

```bash
sudo udevadm control --reload-rules
sudo usermod -aG plugdev $USER
```

### Important — Log Out and Back In

The group change does not take effect until you log out and log
back in. This step is mandatory.

### Verify the Board is Detected

Plug in your Nucleo board via USB, then run:

```bash
lsusb
```

You should see a line containing:

```
Bus 00x Device 00x: ID 0483:374b STMicroelectronics ST-LINK/V2.1
```

If this line appears, the board is recognized and permissions are
configured correctly.

---

## Step 5 — Install VS Code

### Add Microsoft Repository

```bash
sudo apt install wget gpg -y

wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg

sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/

sudo sh -c 'echo "deb [arch=amd64 signed-by=/etc/apt/trusted.gpg.d/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
```

### Install VS Code

```bash
sudo apt update
sudo apt install code -y
```

### Launch VS Code

```bash
code
```

---

## Step 6 — Install VS Code Extensions

Press Ctrl+Shift+X to open the Extensions panel. Search and install:

| Extension    | Author    | Purpose                                    |
|--------------|-----------|--------------------------------------------|
| C/C++        | Microsoft | IntelliSense, syntax highlighting          |
| Cortex-Debug | marus25   | Connects VS Code debugger to OpenOCD       |

---

## Step 7 — Create Project Folder Structure

```bash
mkdir -p ~/stm32-bare-metal/{src,inc,startup,linker}
cd ~/stm32-bare-metal
```

Verify the structure:

```bash
find . -type d
```

Expected output:

```
.
./src
./inc
./startup
./linker
```

Open the project in VS Code:

```bash
code ~/stm32-bare-metal
```

### Folder Purpose

| Folder    | Contents                                              |
|-----------|-------------------------------------------------------|
| src/      | C source files — drivers, main.c                     |
| inc/      | Header files — register definitions, typedefs        |
| startup/  | Assembly startup file — runs before main()           |
| linker/   | Linker script — maps code to Flash and RAM           |

---

## Complete Toolchain Flow

```
You write C
     │
     ▼
arm-none-eabi-gcc compiles
     │
     ▼
.elf binary file
     │
     ▼
OpenOCD + ST-LINK flashes to chip
     │
     ▼
Cortex-Debug lets you set breakpoints
and inspect registers live in VS Code
```

Nothing is hidden. Every step is a command you explicitly run.
No IDE magic. No auto-generated code. Full control.

---

## Quick Reference — All Commands in Order

```bash
# 1. Install toolchain
sudo apt update && sudo apt upgrade -y
sudo apt install gcc-arm-none-eabi openocd make -y

# 2. Verify
arm-none-eabi-gcc --version
openocd --version
make --version

# 3. ST-LINK udev rule
echo 'ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev"' | sudo tee /etc/udev/rules.d/99-stlink.rules
sudo udevadm control --reload-rules
sudo usermod -aG plugdev $USER
# log out and log back in

# 4. Verify board detection
lsusb  # look for ID 0483:374b

# 5. Install VS Code
sudo apt install wget gpg -y
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/
sudo sh -c 'echo "deb [arch=amd64 signed-by=/etc/apt/trusted.gpg.d/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
sudo apt update
sudo apt install code -y

# 6. Create project structure
mkdir -p ~/stm32-bare-metal/{src,inc,startup,linker}
code ~/stm32-bare-metal
```

---

## RAG Metadata

```
topic: bare-metal STM32 environment setup
chip: STM32F446RE
board: NUCLEO-F446RE
os: Ubuntu Linux 22.04 / 24.04
compiler: arm-none-eabi-gcc 13.2.1
debugger: OpenOCD 0.12.0
ide: VS Code
extensions: C/C++ Microsoft, Cortex-Debug marus25
keywords: bare-metal, STM32, arm-none-eabi, OpenOCD, ST-LINK, udev, toolchain
```

# Project Structure & Build System Setup
## (Append to environment_setup.md)

---

## Step 8 — Get CMSIS Headers from ARM

CMSIS (Cortex Microcontroller Software Interface Standard) headers
provide core Cortex-M4 definitions — SysTick, NVIC, core registers.
These are maintained by ARM directly on GitHub.

You do not need the entire repository. Clone it and copy only what
you need:

```bash
# Clone ARM CMSIS into a temporary location
git clone --depth 1 https://github.com/ARM-software/CMSIS_6.git /tmp/cmsis

# Copy only the Cortex-M core headers into your project
cp /tmp/cmsis/CMSIS/Core/Include/core_cm4.h         ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_gcc.h         ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_compiler.h    ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_version.h     ~/stm32-bare-metal/inc/

# Copy M-profile specific headers
mkdir -p ~/stm32-bare-metal/include/m-profile
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv7m_cachel1.h  ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv7m_mpu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv8m_mpu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv8m_pmu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv81m_pac.h     ~/stm32-bare-metal/inc/m-profile/

# Clean up the temporary clone
rm -rf /tmp/cmsis
```

---

## What Each Header File Does

### Files You Must Have

| File                | Purpose                                              |
|---------------------|------------------------------------------------------|
| core_cm4.h          | Cortex-M4 core — NVIC, SysTick, FPU, core registers |
| cmsis_gcc.h         | GCC-specific compiler intrinsics and attributes      |
| cmsis_compiler.h    | Compiler-agnostic wrapper — includes cmsis_gcc.h     |
| cmsis_version.h     | CMSIS version definitions                            |

### Files You Write Yourself

| File                | Purpose                                              |
|---------------------|------------------------------------------------------|
| stm32f446xx.h       | Chip-specific — base addresses, TypeDefs, defines    |
| stm32f4xx.h         | Family header — includes stm32f446xx.h               |
| gpio.h              | GPIO driver declarations                             |
| system_stm32f4xx.h  | System clock init declaration                        |

### stm32f4xx.h — The Family Header

This file's only job is to include the correct chip-specific header.
This is the portability layer — change one line here to target a
different STM32F4 chip:

```c
#ifndef STM32F4XX_H
#define STM32F4XX_H

/* Change this line to target a different chip in the F4 family */
#include "stm32f446xx.h"

#endif
```

### system_stm32f4xx.h — System Clock Declaration

```c
#ifndef SYSTEM_STM32F4XX_H
#define SYSTEM_STM32F4XX_H

#include <stdint.h>

/* Current system clock frequency — updated by SystemInit() */
extern uint32_t SystemCoreClock;

/* Called by startup file before main() — initializes system clock */
void SystemInit(void);

#endif
```

---

## Complete include/ Folder Structure

```
include/
├── m-profile/
│   ├── armv7m_cachel1.h     ← from ARM CMSIS GitHub
│   ├── armv7m_mpu.h         ← from ARM CMSIS GitHub
│   ├── armv8m_mpu.h         ← from ARM CMSIS GitHub
│   ├── armv8m_pmu.h         ← from ARM CMSIS GitHub
│   └── armv81m_pac.h        ← from ARM CMSIS GitHub
├── cmsis_compiler.h         ← from ARM CMSIS GitHub
├── cmsis_gcc.h              ← from ARM CMSIS GitHub
├── cmsis_version.h          ← from ARM CMSIS GitHub
├── core_cm4.h               ← from ARM CMSIS GitHub
├── gpio.h                   ← you write this
├── stm32f4xx.h              ← you write this (family header)
├── stm32f446xx.h            ← you write this (chip header)
└── system_stm32f4xx.h       ← you write this
```

---

## Step 9 — Startup File and Linker Script

### Do Separate Folders Matter

No. The compiler and linker do not care about folder names. They only
care about the paths you give them in the Makefile. Your project
worked without separate folders because the Makefile pointed directly
to the files wherever they were.

Separate folders are a cleanliness convention, not a requirement.
Both structures below are equally valid:

```
# Flat structure — works fine
project/
├── startup_stm32f446xx.s
├── stm32f446re.ld
├── main.c
└── Makefile

# Organized structure — also works fine
project/
├── startup/
│   └── startup_stm32f446xx.s
├── linker/
│   └── stm32f446re.ld
├── src/
│   └── main.c
└── Makefile
```

The only difference is the paths written in the Makefile.

### What the Startup File Does

The startup file runs before main(). It is written in assembly and
performs three jobs:

```
1. Define the vector table — list of function pointers for every
   possible interrupt and exception handler

2. Initialize RAM — copy initial values for global variables from
   Flash into SRAM (the .data section copy)

3. Zero BSS — clear the .bss section (uninitialized globals) to 0

4. Call main()
```

Without the startup file, global variables have garbage values when
main() runs and interrupt handlers have no address to jump to.

### What the Linker Script Does

The linker script answers one question the compiler cannot — where
does each piece of code and data physically live on this chip?

```
Flash starts at 0x08000000 — code goes here
SRAM  starts at 0x20000000 — variables go here
Stack grows downward from top of SRAM
```

The linker script defines these regions and tells the linker exactly
how to arrange your compiled code and data inside them.

---

---

## Step 10 — The Makefile

The Makefile is the build system. It defines exactly how your source
files are compiled, linked, and flashed to the chip. Without it you
would have to type the full compiler command manually every time you
change a single line of code.

```makefile
#------------------------------------------------------------------
# Bare-Metal STM32F446RE Makefile
#------------------------------------------------------------------

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# Target name — output files will be blink.elf and blink.bin
TARGET  = blink

# CPU flags — Cortex-M4 with hardware FPU
CPUFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Compiler flags
CFLAGS  = $(CPUFLAGS)
CFLAGS += -O0                        # no optimization during development
CFLAGS += -Wall                      # all warnings
CFLAGS += -Wextra                    # extra warnings
CFLAGS += -std=c11                   # C11 standard
CFLAGS += -nostdlib                  # no standard C library — we have no OS
CFLAGS += -ffreestanding             # freestanding environment
CFLAGS += -g                         # debug symbols for Cortex-Debug
CFLAGS += -I./include                # header file search path

# Linker flags
LDFLAGS  = $(CPUFLAGS)
LDFLAGS += -T linker/stm32f446re.ld  # linker script location
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections         # strip unused code sections

# Source files
SRCS  = src/main.c
SRCS += src/gpio.c
SRCS += src/system_stm32f4xx.c

# Startup file — assembly, runs before main()
STARTUP = startup/startup_stm32f446xx.s

# Object files derived from source files
OBJS  = $(SRCS:.c=.o)
OBJS += $(STARTUP:.s=.o)

#------------------------------------------------------------------
# Build Rules
#------------------------------------------------------------------

# Default target — builds .elf and .bin
all: $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

# Link all object files into final .elf
$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Convert .elf to raw binary
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Compile each C source file to object file
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble startup file to object file
%.o: %.s
	$(CC) $(CPUFLAGS) -c -o $@ $<

# Flash binary to board using OpenOCD
flash: $(TARGET).bin
	openocd -f interface/stlink.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program $(TARGET).bin 0x08000000 verify reset exit"

# Remove all build artifacts
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin

.PHONY: all flash clean
```

### Makefile Flag Reference

| Flag | Meaning |
|------|---------|
| `-mcpu=cortex-m4` | Target Cortex-M4 CPU |
| `-mthumb` | Thumb-2 instruction set — mandatory for Cortex-M |
| `-mfpu=fpv4-sp-d16` | Enable hardware floating point unit |
| `-mfloat-abi=hard` | Pass floats in FPU registers — faster |
| `-nostdlib` | No standard C library — we have no OS |
| `-ffreestanding` | No assumptions about standard environment |
| `-Wl,--gc-sections` | Strip unused functions from final binary |
| `-g` | Include debug symbols for VS Code debugger |

### Flat Structure vs Subfolder Structure

If you keep startup and linker files in the root folder instead of
subfolders, change only these two lines:

| Item | With Subfolders | Flat Structure |
|------|-----------------|----------------|
| Startup | `startup/startup_stm32f446xx.s` | `startup_stm32f446xx.s` |
| Linker | `-T linker/stm32f446re.ld` | `-T stm32f446re.ld` |

Everything else in the Makefile stays identical.

### Build Commands

```bash
# Compile and link everything
make
```

Expected terminal output:
```
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb ... -c -o src/main.o src/main.c
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb ... -c -o src/gpio.o src/gpio.c
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb ... -c -o startup/startup_stm32f446xx.o startup/startup_stm32f446xx.s
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb ... -o blink.elf src/main.o src/gpio.o startup/startup_stm32f446xx.o
arm-none-eabi-objcopy -O binary blink.elf blink.bin
arm-none-eabi-size blink.elf
   text    data     bss     dec     hex filename
    412       0       0     412     19c blink.elf
```

What each column in the size output means:

| Column | Meaning |
|--------|---------|
| text | Code and read-only data — goes into Flash |
| data | Initialized global variables — copied to SRAM at startup |
| bss | Uninitialized globals — zeroed in SRAM at startup |
| dec | Total size in decimal bytes |

```bash
# Remove all compiled files and start fresh
make clean
```

Expected terminal output:
```
rm -f src/main.o src/gpio.o startup/startup_stm32f446xx.o blink.elf blink.bin
```

---

## Step 11 — Blink Example and Flashing to the Board

### The Blink Example

This is the bare-metal equivalent of Hello World. It blinks the
onboard green LED LD2 connected to PA5 on the NUCLEO-F446RE.

No HAL. No CMSIS driver calls. Pure register manipulation using
the GPIO library we built.

```c
#include <stdint.h>

#define RCC_BASE        0x40023800UL
#define GPIOA_BASE      0x40020000UL

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE   + 0x30))
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14))

static void delay(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    RCC_AHB1ENR  |= (1U << 0);          // enable GPIOA clock
    GPIOA_MODER  &= ~(3U << 10);        // clear PA5 mode bits
    GPIOA_MODER  |=  (1U << 10);        // PA5 = output

    while (1)
    {
        GPIOA_ODR |=  (1U << 5);        // LED ON
        delay(160000);
        GPIOA_ODR &= ~(1U << 5);        // LED OFF
        delay(160000);
    }

    return 0;
}

```

### Plug In the Board

Connect your NUCLEO-F446RE to your PC with a USB cable.
Verify Linux still sees it:

```bash
lsusb
```

Expected output — confirm this line is present:
```
Bus 00x Device 00x: ID 0483:374b STMicroelectronics ST-LINK/V2.1
```

If this line is missing, unplug and replug the board.

### Compile the Project

```bash
make
```

Expected terminal output:
```
arm-none-eabi-gcc ... -c -o src/main.o src/main.c
arm-none-eabi-gcc ... -c -o src/gpio.o src/gpio.c
arm-none-eabi-gcc ... -c -o startup/startup_stm32f446xx.o startup/startup_stm32f446xx.s
arm-none-eabi-gcc ... -o blink.elf src/main.o src/gpio.o startup/startup_stm32f446xx.o
arm-none-eabi-objcopy -O binary blink.elf blink.bin
arm-none-eabi-size blink.elf
   text    data     bss     dec     hex filename
    412       0       0     412     19c blink.elf
```

If you see errors at this stage, they are compiler errors in your
C code. Fix them before proceeding to flash.

### Flash to the Board

```bash
make flash
```

Expected terminal output:
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program blink.bin 0x08000000 verify reset exit"
Open On-Chip Debugger 0.12.0
Info : using stlink api v2
Info : Target voltage: 3.247382
Info : STM32F4xx: hardware has 2 breakpoints, 4 watchpoints
Info : starting gdb server on port 3333
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **
shutdown command invoked
```

Every line of this output matters:

| Line | Meaning |
|------|---------|
| `Target voltage: 3.24` | Board is powered and ST-LINK detected correctly |
| `Programming Started` | OpenOCD is writing blink.bin to Flash at 0x08000000 |
| `Programming Finished` | All bytes written successfully |
| `Verified OK` | OpenOCD read back Flash and confirmed it matches blink.bin |
| `Resetting Target` | Board resets and your code starts running immediately |
| `shutdown command invoked` | OpenOCD disconnects cleanly |

### What You Should See on the Board

After `Resetting Target` appears in the terminal, the green LED LD2
on your Nucleo board begins blinking at approximately 1Hz — on for
10ms, off for 10ms repeatedly.

If the LED does not blink:
- Confirm `Verified OK` appeared in the flash output
- Confirm PA5 is the correct LED pin for your board revision
  (check UM1724 Section 6.6 — User LD2 is connected to PA5)
- Run `make clean` then `make flash` again

### Common Flash Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `libusb_open() failed LIBUSB_ERROR_ACCESS` | udev rule not applied | Log out and back in |
| `Error: open failed` | Board not detected | Check USB cable, run lsusb |
| `Error: init mode failed` | ST-LINK firmware outdated | Update ST-LINK firmware via STM32CubeProgrammer |
| `Verify Failed` | Flash write error | Run make clean, rebuild, flash again |

---

## RAG Metadata

```
topic: STM32 bare-metal Makefile, blink example, flashing with OpenOCD
chip: STM32F446RE
board: NUCLEO-F446RE
led: LD2, PA5
keywords: Makefile, make flash, OpenOCD, blink, gpio, bare-metal,
          arm-none-eabi-gcc, linker script, startup file, flashing
```
