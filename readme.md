# Microcontroller Fundamentals
### A Textbook for Absolute Beginners

---

## Table of Contents

1. [What Is a Microcontroller?](#1-what-is-a-microcontroller)
2. [What's Inside an MCU?](#2-whats-inside-an-mcu)
3. [Memory: Where Code and Data Live](#3-memory-where-code-and-data-live)
4. [How the CPU Executes Your Code](#4-how-the-cpu-executes-your-code)
5. [Peripherals: The Hardware Modules](#5-peripherals-the-hardware-modules)
6. [Registers: The Switchboards](#6-registers-the-switchboards)
7. [The Clock: The Heartbeat of the MCU](#7-the-clock-the-heartbeat-of-the-mcu)
8. [RCC: Turning Peripherals On](#8-rcc-turning-peripherals-on)
9. [Writing to Registers in C](#9-writing-to-registers-in-c)
10. [Making Code Readable: CMSIS Structs](#10-making-code-readable-cmsis-structs)
11. [The Two Documents You Will Always Need](#11-the-two-documents-you-will-always-need)
12. [Putting It All Together: Blink an LED](#12-putting-it-all-together-blink-an-led)

---

## 1. What Is a Microcontroller?

A microcontroller (MCU) is a small, self-contained computer built onto a single chip. It has everything a computer needs — a processor, memory, and input/output pins — but on a much smaller scale and designed to do one specific job.

You have probably already met one. The Arduino is built around a microcontroller. The STM32 family from ST Microelectronics is another popular one. The chip that controls your microwave's buttons and timer is almost certainly a microcontroller.

The key difference from a general-purpose computer (like the one running your browser right now) is purpose. A general-purpose computer runs many different programs for many different tasks. A microcontroller is embedded inside a device — it runs one piece of software, repeatedly, to do one job: read a sensor, drive a motor, blink some lights, control a thermostat.

> **In one sentence:** A microcontroller is a chip that reads inputs, makes decisions, and produces outputs — all in a tight loop, forever.

---

## 2. What's Inside an MCU?

Open up an STM32 datasheet and you will find a block diagram — a picture of all the parts inside the chip and how they connect to each other. Here are the main ones:

### The CPU (Central Processing Unit)
This is the brain. It fetches instructions from memory, figures out what each instruction means, and executes it. On the STM32F446RE, the CPU is an ARM Cortex-M4 core. It can run at up to 180 million instructions per second.

### Flash Memory
This is where your compiled program is permanently stored. It is non-volatile — it survives power loss. When the MCU powers on, the CPU starts reading instructions from Flash.

### SRAM (Static RAM)
This is where your program's variables, the function call stack, and temporary data live at runtime. It is fast, directly accessible, and volatile — when power is cut, everything in SRAM disappears.

### Peripherals
These are hardware modules built into the chip that handle specific tasks: communicating over serial lines, reading analog voltages, driving pins high or low, generating precise timing signals. The CPU talks to them through a shared bus. Examples include GPIO, UART, ADC, and Timers.

### Buses (AHB / APB)
The CPU, memory, and peripherals do not connect to each other directly. They connect through shared buses — high-speed data highways on silicon. AHB (Advanced High-performance Bus) is the fast bus, used by high-speed things like GPIO. APB (Advanced Peripheral Bus) is slower, used by lower-speed peripherals like UART and SPI.

```
                      ┌─────────────┐
                      │     CPU     │
                      └──────┬──────┘
                             │
            ┌────────────────┼────────────────┐
            │                │                │
       ┌────┴─────┐   ┌──────┴──────┐  ┌─────┴────┐
       │  Flash   │   │    SRAM     │  │   AHB /  │
       │ (code)   │   │  (data)     │  │   APB    │
       └──────────┘   └─────────────┘  └──────────┘
                                            │
                        ┌───────────────────┼──────────────────┐
                        │                   │                  │
                   ┌────┴────┐        ┌─────┴─────┐      ┌────┴────┐
                   │  GPIO   │        │   UART    │      │   ADC   │
                   └─────────┘        └───────────┘      └─────────┘
```

---

## 3. Memory: Where Code and Data Live

Here is a concept that surprises most beginners: **in a microcontroller, everything — code, data, and hardware — shares the same address space.**

The 32-bit processor in the STM32 can address 2^32 = 4,294,967,296 unique memory locations. This entire space is divided into regions, each mapped to a specific purpose. Here is a simplified view:

| Address Range | What it maps to |
|---|---|
| `0x08000000` | Flash memory (your compiled code lives here) |
| `0x20000000` | SRAM (your variables live here at runtime) |
| `0x40000000` | Peripheral registers (GPIO, UART, ADC, etc.) |

This means that reading from address `0x08000000` reads a byte of your program's instructions. Reading from address `0x20000000` reads a byte of RAM. And reading or writing to `0x40020000` — which is where GPIOA's registers start — directly interacts with the GPIOA hardware module.

**This design has a name: Memory-Mapped I/O.**

There is no special instruction to "talk to a peripheral." You simply read and write to specific memory addresses, and the hardware responds.

---

## 4. How the CPU Executes Your Code

When you power on the MCU, the CPU goes through the same loop forever, one iteration per clock tick:

**Step 1 — Fetch:** The CPU reads the next instruction from Flash. It knows which instruction to read because it tracks a special register called the Program Counter (PC), which always holds the address of the next instruction.

**Step 2 — Decode:** The CPU looks at the raw binary it just fetched and figures out what it means. Is it an addition? A comparison? A jump to another address? A write to memory?

**Step 3 — Execute:** The CPU performs the operation. If it's an arithmetic operation, the ALU (Arithmetic Logic Unit) computes the result. If it's a memory write, the value gets sent out to the right address on the bus.

Then the PC advances to the next instruction, and the whole thing repeats. On a 180 MHz CPU, this happens 180 million times per second.

> **The important insight:** Your entire C program — `if` statements, `for` loops, function calls, everything — eventually becomes a sequence of these simple fetch-decode-execute steps. The compiler translates your high-level code into machine instructions, and the CPU just runs through them one by one.

---

## 5. Peripherals: The Hardware Modules

A peripheral is a hardware circuit built into the MCU that handles a specific job. The CPU alone is just a calculator — it can only do math and move data around. Peripherals are what make an MCU actually interact with the physical world.

Here are the most important ones you will encounter:

### GPIO — General Purpose Input/Output
GPIO pins are the simplest peripheral. Each pin can be configured as either an output (you drive it high or low to control something) or an input (you read its voltage to detect something).

Set a pin high → LED turns on.
Set a pin low → LED turns off.
Read a pin → find out if a button is pressed.

On the STM32, there are multiple GPIO ports: GPIOA, GPIOB, GPIOC, and so on. Each port controls up to 16 physical pins (A0–A15, B0–B15, etc.).

### UART — Universal Asynchronous Receiver/Transmitter
UART lets the MCU send and receive text (or any bytes) over two wires: TX (transmit) and RX (receive). This is how you print debug messages to your computer's terminal. When you use `printf` in embedded code, it usually goes through UART.

### ADC — Analog to Digital Converter
Most of the real world is analog — voltages that vary continuously. A microphone produces a varying voltage. A temperature sensor produces a varying voltage. The ADC samples that voltage and converts it to a digital number your CPU can work with. On a 12-bit ADC, the output is a number from 0 to 4095, representing a voltage from 0 to 3.3V.

### Timers and PWM
Timers let you measure precise time intervals or trigger actions after a delay. PWM (Pulse Width Modulation) uses a timer to produce a signal that switches on and off very fast at a controllable duty cycle — this is how you dim an LED or control a servo motor.

Each of these peripherals is controlled entirely through its registers.

---

## 6. Registers: The Switchboards

A register is a 32-bit memory location at a fixed address. It is the interface between your software and the hardware peripheral.

Think of a register as a row of 32 light switches. Each switch (bit) controls something specific about the hardware's behaviour. Flip the right switches in the right combination, and the hardware does what you want.

For example, GPIOA has a register called **MODER** (Mode Register) at address `0x40020000 + 0`. Its job is to configure the mode of each pin. The 32 bits are split into 16 pairs — each pair controls one pin:

```
Bits 1:0   → Pin A0 mode
Bits 3:2   → Pin A1 mode
Bits 5:4   → Pin A2 mode
Bits 7:6   → Pin A3 mode
...and so on
```

The 2-bit value for each pin means:
- `00` = input mode
- `01` = output mode
- `10` = alternate function (UART, SPI, I2C, etc.)
- `11` = analog mode (for ADC)

So if you want to make pin A5 an output, you need to set bits 11:10 (the pair for pin 5) to `01`. Every other bit should stay as it is — you don't want to accidentally change the configuration of other pins.

This is where the clear-then-set pattern comes in.

---

## 7. The Clock: The Heartbeat of the MCU

A clock signal is a square wave — it switches between high and low voltage at a fixed frequency. Every flip-flop and latch in the chip changes state on the rising edge of this signal. Without a clock, nothing happens. Everything waits.

On the STM32, the clock comes from a source (the internal 16 MHz RC oscillator, or an external crystal), gets multiplied by a PLL (Phase-Locked Loop), and the output — called SYSCLK — is distributed throughout the chip.

Here is the critical rule:

> **The clock signal is not automatically supplied to peripherals. Each peripheral's clock is off by default.**

This is a deliberate design choice to save power. Unused peripherals draw no power because their clocks are gated off.

Before you can use any peripheral, you must explicitly turn on its clock supply. If you do not, writing to that peripheral's registers has absolutely no effect. The peripheral is asleep.

This is almost certainly the most common first bug when learning embedded programming: the code looks correct, but nothing happens — because the peripheral's clock was never enabled.

---

## 8. RCC: Turning Peripherals On

The RCC (Reset and Clock Control) block is the gatekeeper. It has a set of registers, each bit of which controls the clock supply to one peripheral.

For the STM32F446, GPIO ports are on the AHB1 bus. So to enable GPIOA, you set the corresponding bit in the **RCC_AHB1ENR** register (AHB1 Enable Register). UART peripherals sit on APB buses, so they have their own enable bits in **RCC_APB1ENR** or **RCC_APB2ENR**.

The process for every single peripheral you will ever use is:

```
1. Find which bus the peripheral is on (from the datasheet).
2. Set the corresponding enable bit in the matching RCC_xxxENR register.
3. Now you can configure and use the peripheral.
```

If you skip step 2, step 3 silently does nothing.

---

## 9. Writing to Registers in C

Now that you understand what registers are, how do you actually write to them in C?

Since a register is just a memory location with a fixed address, you write to it using a pointer. Here is the raw form:

```c
// Write the value 0 to the address 0x40020000 (GPIOA MODER)
*(volatile uint32_t *)(0x40020000) = 0;
```

Breaking this down:
- `0x40020000` is the address of the GPIOA MODER register
- `(volatile uint32_t *)` casts that address to a pointer to a 32-bit unsigned integer
- The `*` at the front dereferences the pointer — it says "write to this location, not to the variable holding the address"
- `volatile` tells the compiler: do not optimize this away, do not reorder it, always read/write this location directly

### Why `volatile`?

The C compiler is smart and lazy. If you write a value to a variable and never read it back in your code, the compiler assumes it's pointless and removes the write. But peripheral registers are not ordinary variables — writing to them changes hardware behaviour. The compiler doesn't know that. `volatile` is your way of saying: this location affects the outside world, always honour every read and write.

### The Clear-Then-Set Pattern

You almost never want to write the entire register at once, because that would reset all other bits to zero. Instead, you modify only the bits you care about, leaving the rest untouched.

This is done in two steps:

**Step 1 — Clear the target bits:**

```c
*(volatile uint32_t *)(0x40020000) &= ~(3 << 6);
```

This clears bits 6 and 7 (the MODER bits for pin A3) without touching any other bits. Here is the logic:

| Step | Expression | What it does |
|---|---|---|
| Start with N ones | `3` | `...000011` |
| Shift left by X | `3 << 6` | `...11000000` |
| Invert | `~(3 << 6)` | `...00111111` |
| AND with register | `REG &= ~(3 << 6)` | Zeros only bits 6–7, leaves everything else |

**Step 2 — Set the target bits to the desired value:**

```c
*(volatile uint32_t *)(0x40020000) |= (1 << 6);
```

This ORs the value `1` into position 6, setting bits 6–7 to `01` (output mode), while leaving all other bits unchanged.

The general pattern is:

```c
// To set N bits at position X to value V:
REGISTER &= ~((0xF) << X);   // clear the bits first (mask size depends on N)
REGISTER |= (V << X);        // then set them to V
```

---

## 10. Making Code Readable: CMSIS Structs

Raw address arithmetic is correct but unreadable. Compare these two lines — they do exactly the same thing:

```c
// Raw address access
*(volatile uint32_t *)(0x40020000 + 0) |= (1 << 10);

// Struct-based access
GPIOA->MODER |= (1 << 10);
```

The second form is far clearer. The way this works is by mapping the peripheral's registers onto a C struct.

Looking at GPIOA's register layout from the reference manual:

| Register | Offset |
|---|---|
| MODER | 0x00 |
| OTYPER | 0x04 |
| OSPEEDR | 0x08 |
| PUPDR | 0x0C |
| IDR | 0x10 |
| ODR | 0x14 |
| BSRR | 0x18 |
| ... | ... |

Because the registers are laid out sequentially, they map directly to fields of a struct:

```c
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

#define GPIOA  ((GPIO_TypeDef *) 0x40020000)
```

Now `GPIOA->MODER` is exactly the register at address `0x40020000 + 0`. `GPIOA->ODR` is at `0x40020000 + 0x14`. The struct layout does the offset arithmetic for you, and the field names come from the reference manual, so you always know what you're touching.

The CMSIS headers (provided by ARM and ST) define all of these structs and macros for you. Every peripheral, every register, every bit field — all named exactly as they appear in the reference manual.

---

## 11. The Two Documents You Will Always Need

### The Datasheet
Describes the hardware: what pins the chip has, what alternate functions each pin supports, the electrical characteristics, and — critically — the memory map (every peripheral's base address). It tells you what the chip **is**.

Use the datasheet to answer: *Which pin do I use? What address does this peripheral start at? Which bus is it on?*

### The Reference Manual
Describes how to use every peripheral in detail: which registers exist, what each bit controls, and in what order you must configure things. It tells you how to **operate** the chip.

Use the reference manual to answer: *Which bits do I set? What sequence do I follow? What does this register do?*

For the STM32F446RE:
- Datasheet: search for **STM32F446RE datasheet** on st.com
- Reference Manual: search for **RM0390** on st.com

These are not optional reading. They are your primary source of truth. The code you write is a direct translation of what these documents say. When something doesn't work, the answer is almost always in one of these two files.

---

## 12. Putting It All Together: Blink an LED

Here is the complete thought process for the simplest possible embedded program — blinking an LED on pin PA5 of the STM32F446RE Nucleo board.

### Step 1: Identify the hardware
From the Nucleo board schematic, the user LED (LD2) is connected to pin PA5 (GPIO port A, pin 5).

### Step 2: Enable the clock for GPIOA
GPIOA is on the AHB1 bus. Look up `RCC_AHB1ENR` in the reference manual. Bit 0 controls the clock for GPIOA.

```c
RCC->AHB1ENR |= (1 << 0);   // Enable clock for GPIOA
```

### Step 3: Configure PA5 as output
The MODER register controls pin modes. Pin 5's bits are at positions 11:10. Set them to `01` (output mode).

```c
GPIOA->MODER &= ~(3 << 10);  // Clear bits 11:10
GPIOA->MODER |=  (1 << 10);  // Set bits 11:10 to 01 (output)
```

### Step 4: Toggle the pin in a loop
The ODR (Output Data Register) controls the output state of each pin. Bit 5 controls PA5.

```c
while (1) {
    GPIOA->ODR |=  (1 << 5);   // Set PA5 high → LED on
    delay();
    GPIOA->ODR &= ~(1 << 5);   // Set PA5 low  → LED off
    delay();
}
```

### The complete program structure

```c
#include <stdint.h>

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t          RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t          RESERVED1[2];
    volatile uint32_t AHB1ENR;   // offset 0x30 — what we need
} RCC_TypeDef;

#define GPIOA  ((GPIO_TypeDef *) 0x40020000)
#define RCC    ((RCC_TypeDef *)  0x40023800)

static void delay(void) {
    for (volatile int i = 0; i < 100000; i++);
}

int main(void) {
    // 1. Enable GPIOA clock
    RCC->AHB1ENR |= (1 << 0);

    // 2. Set PA5 to output mode
    GPIOA->MODER &= ~(3 << 10);
    GPIOA->MODER |=  (1 << 10);

    // 3. Blink
    while (1) {
        GPIOA->ODR |=  (1 << 5);
        delay();
        GPIOA->ODR &= ~(1 << 5);
        delay();
    }
}
```

Every line here is a direct consequence of reading the datasheet and reference manual. There is no magic. No library is doing anything hidden. You are talking directly to the hardware.

---

## Summary

| Concept | What it is | Why it matters |
|---|---|---|
| MCU | A small computer on a chip | The thing you are programming |
| Flash | Non-volatile code storage | Where your compiled program lives |
| SRAM | Volatile runtime memory | Where variables and the stack live |
| Peripheral | Hardware module inside the MCU | What does the actual real-world work |
| Register | 32-bit word at a fixed address | How you configure and control hardware |
| Memory-mapped I/O | Peripherals live in the address space | You control hardware by reading/writing addresses |
| `volatile` | Tells the compiler not to optimize away | Required for all register accesses |
| Clear-then-set | `&= ~(mask)` then `|= value` | How you safely modify individual bits |
| RCC | Clock controller | Must be enabled before any peripheral works |
| Clock | Square wave that synchronises everything | Nothing works without it |
| Datasheet | Hardware description | Tells you what exists and where |
| Reference Manual | Peripheral operation guide | Tells you how to use it |

---

*This document covers the STM32F446RE and is consistent with RM0390 (Reference Manual) and the STM32F446RE datasheet. All register names and addresses reflect those documents.*

