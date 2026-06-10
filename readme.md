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
