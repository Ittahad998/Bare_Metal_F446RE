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

---

## Step 7 — Create Project Folder Structure

```bash
mkdir -p ~/stm32-bare-metal/{src,inc,inc/m-profile,startup,linker}
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
./inc/m-profile
./startup
./linker
```

Open the project in VS Code:

```bash
code ~/stm32-bare-metal
```

### Folder Purpose

| Folder        | Contents                                          |
|---------------|---------------------------------------------------|
| src/          | C source files — drivers, main.c                 |
| inc/          | Header files — register definitions, typedefs    |
| inc/m-profile | ARM Cortex-M profile specific headers            |
| startup/      | Assembly startup file — runs before main()       |
| linker/       | Linker script — maps code to Flash and RAM       |

---

## Step 8 — Get CMSIS Headers From ARM GitHub

```bash
git clone --depth 1 https://github.com/ARM-software/CMSIS_6.git /tmp/cmsis

# Core headers
cp /tmp/cmsis/CMSIS/Core/Include/core_cm4.h          ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_gcc.h          ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_compiler.h     ~/stm32-bare-metal/inc/
cp /tmp/cmsis/CMSIS/Core/Include/cmsis_version.h      ~/stm32-bare-metal/inc/

# M-profile specific headers
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv7m_cachel1.h  ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv7m_mpu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv8m_mpu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv8m_pmu.h      ~/stm32-bare-metal/inc/m-profile/
cp /tmp/cmsis/CMSIS/Core/Include/m-profile/armv81m_pac.h     ~/stm32-bare-metal/inc/m-profile/

rm -rf /tmp/cmsis
```

---

## Step 9 — Get ST Device Files From ST GitHub

```bash
git clone --depth 1 https://github.com/STMicroelectronics/cmsis-device-f4.git /tmp/stm32f4

# Chip and family headers
cp /tmp/stm32f4/Include/stm32f4xx.h          ~/stm32-bare-metal/inc/
cp /tmp/stm32f4/Include/stm32f446xx.h        ~/stm32-bare-metal/inc/
cp /tmp/stm32f4/Include/system_stm32f4xx.h   ~/stm32-bare-metal/inc/

# System init source
cp /tmp/stm32f4/Source/Templates/system_stm32f4xx.c   ~/stm32-bare-metal/src/

# Startup file
cp /tmp/stm32f4/Source/Templates/gcc/startup_stm32f446xx.s   ~/stm32-bare-metal/startup/

rm -rf /tmp/stm32f4
```

### Get Linker Script From ST CubeF4

```bash
git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF4.git /tmp/cubef4

cp "/tmp/cubef4/Projects/NUCLEO-F446RE/Templates/STM32CubeIDE/STM32F446RETX_FLASH.ld" \
   ~/stm32-bare-metal/linker/stm32f446re.ld

rm -rf /tmp/cubef4
```

### What Each File Does

| File | Source | Purpose |
|------|--------|---------|
| `core_cm4.h` | ARM CMSIS | Cortex-M4 core — NVIC, SysTick, FPU |
| `cmsis_gcc.h` | ARM CMSIS | GCC compiler intrinsics |
| `cmsis_compiler.h` | ARM CMSIS | Compiler-agnostic wrapper |
| `cmsis_version.h` | ARM CMSIS | CMSIS version definitions |
| `stm32f4xx.h` | ST GitHub | Family header — includes stm32f446xx.h |
| `stm32f446xx.h` | ST GitHub | Chip header — all peripheral definitions |
| `system_stm32f4xx.h` | ST GitHub | SystemInit declaration |
| `system_stm32f4xx.c` | ST GitHub | SystemInit implementation |
| `startup_stm32f446xx.s` | ST GitHub | Startup — vector table, RAM init, calls main |
| `stm32f446re.ld` | ST CubeF4 | Linker script — Flash and RAM regions |
| `gpio.h` | You write | GPIO driver declarations |
| `gpio.c` | You write | GPIO driver implementation |
| `main.c` | You write | Your application |

### Complete Project Structure After Steps 7, 8, 9

```
stm32-bare-metal/
├── inc/
│   ├── m-profile/
│   │   ├── armv7m_cachel1.h       ← ARM CMSIS GitHub
│   │   ├── armv7m_mpu.h           ← ARM CMSIS GitHub
│   │   ├── armv8m_mpu.h           ← ARM CMSIS GitHub
│   │   ├── armv8m_pmu.h           ← ARM CMSIS GitHub
│   │   └── armv81m_pac.h          ← ARM CMSIS GitHub
│   ├── cmsis_compiler.h           ← ARM CMSIS GitHub
│   ├── cmsis_gcc.h                ← ARM CMSIS GitHub
│   ├── cmsis_version.h            ← ARM CMSIS GitHub
│   ├── core_cm4.h                 ← ARM CMSIS GitHub
│   ├── stm32f4xx.h                ← ST GitHub
│   ├── stm32f446xx.h              ← ST GitHub
│   ├── system_stm32f4xx.h         ← ST GitHub
│   └── gpio.h                     ← you write this
├── src/
│   ├── system_stm32f4xx.c         ← ST GitHub
│   ├── gpio.c                     ← you write this
│   └── main.c                     ← you write this
├── startup/
│   └── startup_stm32f446xx.s      ← ST GitHub
├── linker/
│   └── stm32f446re.ld             ← ST CubeF4 GitHub
└── Makefile                       ← you write this
```

---

## Step 10 — The Makefile

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
CFLAGS += -nostdlib                  # no standard C library
CFLAGS += -ffreestanding             # freestanding environment
CFLAGS += -g                         # debug symbols for Cortex-Debug
CFLAGS += -I./inc                    # header file search path

# Linker flags
LDFLAGS  = $(CPUFLAGS)
LDFLAGS += -T linker/stm32f446re.ld  # linker script
LDFLAGS += -nostdlib
LDFLAGS += -Wl,--gc-sections         # strip unused code sections

# Source files
SRCS  = src/main.c
SRCS += src/gpio.c
SRCS += src/system_stm32f4xx.c

# Startup file
STARTUP = startup/startup_stm32f446xx.s

# Object files
OBJS  = $(SRCS:.c=.o)
OBJS += $(STARTUP:.s=.o)

#------------------------------------------------------------------
# Build Rules
#------------------------------------------------------------------

all: $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CPUFLAGS) -c -o $@ $<

flash: $(TARGET).bin
	openocd -f interface/stlink.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program $(TARGET).bin 0x08000000 verify reset exit"

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
| `-I./inc` | Tell compiler where to find header files |

### Build Commands

```bash
make
```

Expected output:

```
arm-none-eabi-gcc ... -c -o src/main.o src/main.c
arm-none-eabi-gcc ... -c -o src/gpio.o src/gpio.c
arm-none-eabi-gcc ... -c -o src/system_stm32f4xx.o src/system_stm32f4xx.c
arm-none-eabi-gcc ... -c -o startup/startup_stm32f446xx.o startup/startup_stm32f446xx.s
arm-none-eabi-gcc ... -o blink.elf src/main.o src/gpio.o src/system_stm32f4xx.o startup/startup_stm32f446xx.o
arm-none-eabi-objcopy -O binary blink.elf blink.bin
arm-none-eabi-size blink.elf
   text    data     bss     dec     hex filename
    412       0       0     412     19c blink.elf
```

```bash
make clean
```

Expected output:

```
rm -f src/main.o src/gpio.o src/system_stm32f4xx.o startup/startup_stm32f446xx.o blink.elf blink.bin
```

---

## RAG Metadata

```
topic: STM32 bare-metal project structure, CMSIS setup, Makefile
chip: STM32F446RE
board: NUCLEO-F446RE
os: Ubuntu Linux
folder: inc not include
keywords: CMSIS, Makefile, linker script, startup file, arm-none-eabi-gcc,
          project structure, build system, OpenOCD, ST GitHub, ARM GitHub
```

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

void SystemInit(void) {}   /* called by startup before main()     */
void _init(void)      {}   /* called by __libc_init_array()       */

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
