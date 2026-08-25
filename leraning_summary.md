# STM32F446RE Bare-Metal Embedded Systems

> **From Zero to Hardware Driver Understanding**

| System Specifications | Details |
| --- | --- |
| **Board** | STM32 Nucleo-64 (`NUCLEO-F446RE`) |
| **Chip** | STM32F446RE (ARM Cortex-M4, up to 180MHz, 512KB Flash, 128KB SRAM) |
| **Clock** | 16MHz HSI internal oscillator (default at reset) |
| **Tools** | STM32CubeIDE, `arm-none-eabi-gcc`, RM0390 Reference Manual, STM32F446RE Datasheet, DUI0553 Cortex-M4 User Guide |

---

## Foundational Documentation Navigation

Before writing any code, navigate through these three core reference documents:

### 1. STM32F446RE Datasheet

* **Used for:** Physical pin locations, alternate function mapping, package diagrams, ADC channel-to-pin mapping.
* **Key Table:** *Table 11 - Pin and ball definitions*.

### 2. RM0390 Reference Manual

* **Used for:** Peripheral registers and memory maps.
* **Section 2.2:** Memory map (base addresses and bus assignments)
* **Section 6.3:** RCC (clock enable registers)
* **Section 7.4:** GPIO registers
* **Section 10.3:** EXTI registers
* **Section 13:** ADC registers
* **Section 17.4:** Timer registers
* **Section 25.6:** USART registers
* **Chapter 8:** SYSCFG registers
* **Chapter 9:** DMA registers

### 3. DUI0553 Cortex-M4 User Guide

* **Used for:** ARM core peripherals not documented by ST.
* **Section 4.2:** NVIC registers
* **Section 4.4:** SysTick registers
* **Section 2.3:** Vector table layout

### Standard Hardware Navigation Workflow

```
[Memory Map (Sec 2.2)] ---> Find base address & target bus
         │
         ▼
  [RCC Chapter]        ---> Enable peripheral clock bit
         │
         ▼
[Register Summary]     ---> Determine register offsets
         │
         ▼
[Register Description] ---> Identify bit definitions & flags
         │
         ▼
[Functional Overview]  ---> Review operating behavior

```

---

## Lesson 1: GPIO Output

### Concepts

* GPIO pins are memory-mapped hardware registers. Writing to a register physically changes voltage on a chip pin.
* Every peripheral is frozen by default; its clock must be enabled first.

### Key Registers

* `RCC_AHB1ENR`: Enable clock to GPIO port (bit 0=A, 1=B, 2=C, etc.)
* `GPIOx_MODER`: Set pin mode (`00`=input, `01`=output, `10`=AF, `11`=analog)
* `GPIOx_ODR`: Write 1 or 0 to drive pin HIGH or LOW
* `GPIOx_OTYPER`: Output type (`0`=push-pull, `1`=open-drain)

### Implementation Example (PA5 LED)

```c
#include <stdint.h>

#define RCC_BASE        0x40023800U
#define GPIOA_BASE      0x40020000U

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30U))
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00U))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14U))

void gpio_output_init(void) {
    // 1. Enable GPIOA clock (Bit 0)
    RCC_AHB1ENR |= (1U << 0);

    // 2. Set PA5 as general-purpose output (01 into bits 11:10)
    GPIOA_MODER &= ~(3U << (5 * 2));
    GPIOA_MODER |=  (1U << (5 * 2));
}

void led_on(void) {
    GPIOA_ODR |= (1U << 5);
}

void led_off(void) {
    GPIOA_ODR &= ~(1U << 5);
}

```

### Key Concepts

* **Memory-Mapped I/O:** Registers are accessed via standard memory addresses.
* **`volatile` Keyword:** Prevents the compiler from optimizing away register reads/writes.
* **Bitwise Clear-Set Pattern:** Always perform `&= ~mask` before `|= value`.
* **Clock Gating:** Peripherals remain disabled until enabled in the RCC.

---

## Lesson 2: GPIO Input

### Concepts

* Reading pin state requires reading the Input Data Register (`IDR`).
* Floating pins pick up electrical noise; pull-up or pull-down resistors are required.
* Button PC13 on the Nucleo has an external pull-up (reads `HIGH` unpressed, `LOW` pressed).

### Key Registers

* `GPIOx_IDR`: Read-only. Bit $N$ reflects the physical voltage on pin $N$.
* `GPIOx_PUPDR`: Pull resistor selection (`00`=none, `01`=pull-up, `10`=pull-down).
* `GPIOx_MODER`: Set to `00` for input mode.

### Implementation Example (PC13 Button Reading)

```c
#include <stdint.h>
#include <stdbool.h>

#define GPIOC_BASE      0x40020800U
#define GPIOC_IDR       (*(volatile uint32_t *)(GPIOC_BASE + 0x10U))

bool read_button_pc13(void) {
    // Returns true if pressed (Active LOW)
    return !(GPIOC_IDR & (1U << 13));
}

```

### Key Concepts

* **Read-Only Access:** Writing to `IDR` has no hardware effect.
* **Polling:** Continuously reading pin state in a loop prevents the CPU from performing other background tasks efficiently.

---

## Lesson 3: SysTick Timer

### Concepts

* SysTick is a 24-bit countdown timer built directly into the ARM Cortex-M4 core (documented in DUI0553).
* Counts down from a reload value to zero, fires an interrupt, reloads, and repeats.

### Key Registers

| Register | Address | Description |
| --- | --- | --- |
| `SYST_CSR` | `0xE000E010` | Control and Status (`ENABLE` bit 0, `TICKINT` bit 1, `CLKSOURCE` bit 2) |
| `SYST_RVR` | `0xE000E014` | Reload Value Register (Set to $N-1$ for $N$ cycles) |
| `SYST_CVR` | `0xE000E018` | Current Value Register (Write any value to clear to zero) |

### Implementation Example (1ms Timebase at 16MHz)

```c
#include <stdint.h>

#define SYST_CSR        (*(volatile uint32_t *)0xE000E010U)
#define SYST_RVR        (*(volatile uint32_t *)0xE000E014U)
#define SYST_CVR        (*(volatile uint32_t *)0xE000E018U)

static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

void systick_init(void) {
    SYST_RVR = 16000U - 1U; // 16,000 cycles = 1ms at 16MHz
    SYST_CVR = 0U;
    SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2); // Processor clock + Interrupt + Enable
}

void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

```

---

## Lesson 4: USART Serial Communication

### Concepts

* Hardware-driven serial communication using dedicated transmit/receive shift registers.
* Configured on `PA2` (`USART2_TX`) and `PA3` (`USART2_RX`), which route directly to the ST-LINK Virtual COM Port.

### Alternate Function Mapping

Setting `MODER = 0b10` routes control of the physical pin to internal peripheral lines via `AFRL`/`AFRH` multiplexing.

```
       ┌────────────────────────┐
       │   GPIO Mode (MODER)    │
       └───────────┬────────────┘
                   │ 0b10 (Alternate Function Mode)
                   ▼
┌──────────────────────────────────────┐
│       AF Multiplexer (AFRL/AFRH)     │
├──────────────────┬───────────────────┤
│ AF7 = USART2_TX  │ AF7 = USART2_RX   │
└─────────┬────────┴─────────┬─────────┘
          │                  │
          ▼                  ▼
     PA2 (Pin 12)       PA3 (Pin 13)

```

### Key Registers

* `RCC_APB1ENR`: Bit 17 (`USART2EN`)
* `USART_BRR`: Baud Rate Register ($\text{BRR} = \frac{f_{\text{CK}}}{16 \times \text{Baud}}$)
* `USART_CR1`: Control Register 1 (`UE` bit 13, `TE` bit 3, `RE` bit 2)
* `USART_SR`: Status Register (`TXE` bit 7, `RXNE` bit 5)
* `USART_DR`: Data Register

### Implementation Example

```c
#include <stdint.h>

#define USART2_BASE     0x40004400U
#define USART2_SR       (*(volatile uint32_t *)(USART2_BASE + 0x00U))
#define USART2_DR       (*(volatile uint32_t *)(USART2_BASE + 0x04U))
#define USART2_BRR      (*(volatile uint32_t *)(USART2_BASE + 0x08U))
#define USART2_CR1      (*(volatile uint32_t *)(USART2_BASE + 0x0CU))

void usart2_init(void) {
    // Baud rate setup: 115200 at 16MHz -> BRR = 0x008B
    USART2_BRR = 0x008BU;
    USART2_CR1 = (1U << 3) | (1U << 2) | (1U << 13); // TE, RE, UE
}

void usart2_write_char(char ch) {
    while (!(USART2_SR & (1U << 7))); // Wait for TXE
    USART2_DR = (uint8_t)ch;
}

```

---

## Lesson 5: EXTI (External Interrupts)

### Hardware Architecture Pipeline

```
[GPIO Pin PC13] ---> [SYSCFG EXTICR4] ---> [EXTI Line 13] ---> [NVIC IRQ 40] ---> CPU Handler

```

### Implementation Example

```c
#include <stdint.h>

#define SYSCFG_BASE     0x40013800U
#define EXTI_BASE       0x40013C00U
#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104U)

#define SYSCFG_EXTICR4  (*(volatile uint32_t *)(SYSCFG_BASE + 0x14U))
#define EXTI_IMR        (*(volatile uint32_t *)(EXTI_BASE + 0x00U))
#define EXTI_FTSR       (*(volatile uint32_t *)(EXTI_BASE + 0x0CU))
#define EXTI_PR         (*(volatile uint32_t *)(EXTI_BASE + 0x14U))

void exti13_init(void) {
    // 1. Connect PC13 to EXTI Line 13 via SYSCFG
    SYSCFG_EXTICR4 &= ~(0xFU << 4);
    SYSCFG_EXTICR4 |=  (0x2U << 4); // Port C

    // 2. Configure EXTI Line 13
    EXTI_FTSR |= (1U << 13); // Falling edge trigger
    EXTI_IMR  |= (1U << 13); // Unmask line 13

    // 3. Enable EXTI15_10 IRQ in NVIC (IRQ 40 -> ISER1 Bit 8)
    NVIC_ISER1 |= (1U << (40 - 32));
}

void EXTI15_10_IRQHandler(void) {
    if (EXTI_PR & (1U << 13)) {
        // Clear pending bit by writing 1 to it
        EXTI_PR |= (1U << 13);
        
        // Interrupt logic here
    }
}

```

---

## Lesson 6: Analog-to-Digital Converter (ADC)

### Concepts

* Converted analog signal ranges (0V to 3.3V) into a 12-bit digital value (0 to 4095).
* Single Successive Approximation Register (SAR) core shared across channels via an internal Analog Multiplexer (AMUX).

### Comparison of ADC Reading Methods

| Method | CPU Overhead | Use Case | Multi-Channel Performance |
| --- | --- | --- | --- |
| **Polling** | High (100% busy wait) | Single diagnostic read | Poor (Blocks system execution) |
| **Interrupt** | Medium (Context switch cost) | Low-frequency periodic sampling | Fair (High interrupt rate at speed) |
| **DMA + Circular** | Low (Zero CPU intervention) | Continuous high-speed acquisition | Best (Automated hardware streaming) |

---

## Lesson 7: Direct Memory Access (DMA)

### Concepts

* Hardware block offloading memory transfer tasks from the main CPU.
* ADC converts $\rightarrow$ DMA transfer to SRAM array $\rightarrow$ Buffer wraps automatically in circular mode.

### Implementation Example (DMA Setup for ADC1)

```c
#include <stdint.h>

#define DMA2_BASE       0x40026400U
#define DMA2_S0CR       (*(volatile uint32_t *)(DMA2_BASE + 0x10U))
#define DMA2_S0NDTR     (*(volatile uint32_t *)(DMA2_BASE + 0x14U))
#define DMA2_S0PAR      (*(volatile uint32_t *)(DMA2_BASE + 0x18U))
#define DMA2_S0M0AR     (*(volatile uint32_t *)(DMA2_BASE + 0x1CU))

#define ADC1_DR_ADDR    0x4001204CU
#define ADC_CHANNELS    8U

static volatile uint16_t adc_buffer[ADC_CHANNELS];

void dma2_stream0_adc_init(void) {
    DMA2_S0CR &= ~(1U << 0); // Disable stream during configuration
    while(DMA2_S0CR & (1U << 0));

    DMA2_S0PAR  = (uint32_t)ADC1_DR_ADDR;
    DMA2_S0M0AR = (uint32_t)adc_buffer;
    DMA2_S0NDTR = ADC_CHANNELS;

    // Channel 0, High Priority, 16-bit PSIZE/MSIZE, Memory Increment, Circular Mode
    DMA2_S0CR = (0U << 25) | (2U << 16) | (1U << 13) | 
                (1U << 11) | (1U << 10) | (1U << 8);

    DMA2_S0CR |= (1U << 0); // Enable stream
}

```

---

## Lesson 8: Memory Map & Flashing Architecture

### Physical Memory Layout

```
0x00000000  ┌────────────────────────────────────────┐
            │ Boot Memory / Vector Table Alias       │
0x08000000  ├────────────────────────────────────────┤
            │ Main Flash Memory (512KB)              │
            │ Application Binary & Vector Table      │
0x1FFF0000  ├────────────────────────────────────────┤
            │ System Memory (ST Bootloader, ~30KB)   │
0x20000000  ├────────────────────────────────────────┤
            │ SRAM (128KB Runtime Data)              │
0x40000000  ├────────────────────────────────────────┤
            │ Peripheral Registers Space             │
0xFFFFFFFF  └────────────────────────────────────────┘

```

### Flashing Configurations

```
               ┌────────────────────────┐
               │     RESET SEQUENCE     │
               └───────────┬────────────┘
                           │
                 Check BOOT0 Pin State
                           │
         ┌─────────────────┴─────────────────┐
         │                                   │
   BOOT0 = LOW                         BOOT0 = HIGH
         │                                   │
         ▼                                   ▼
  Execute Application                Execute ST Bootloader
    (Flash 0x08000000)                (System Memory 0x1FFF0000)
         │                                   │
         ▼                                   ▼
 Standard SWD Operations            UART/USB Firmware Updates

```

---

## Quick Reference: Document Navigation Index

```
QUESTION                                   DOCUMENT & SECTION
─────────────────────────────────────────────────────────────────────────
Which pin supports which function?         Datasheet Table 11
Which AF number for which peripheral?      Datasheet Table 11 (AF columns)
Which bus does peripheral X use?           RM0390 Section 2.2 Table 1
Base address of peripheral X?              RM0390 Section 2.2 Table 1
Clock enable bit for peripheral X?         RM0390 Section 6.3 (RCC chapter)
Register offsets for peripheral X?         RM0390 Peripheral Register Map
Bit definitions in register Y?             RM0390 Register Description
SysTick Core registers?                    DUI0553 Section 4.4
NVIC Interrupt controller registers?       DUI0553 Section 4.2
Vector Table IRQ assignments?              RM0390 Table 38
DMA stream for ADC1?                       RM0390 Table 29

```

---

## Learning Roadmap

```
  [COMPLETED]                       [NEXT STEPS]
  ├── GPIO Output (LED)             ├── Multi-Channel ADC + DMA Implementation
  ├── GPIO Input (Button)           ├── Advanced Timers & PWM Generation
  ├── SysTick Microsecond Timebase  ├── CMSIS Structure & Driver Refactoring
  ├── USART Peripheral Drivers      ├── Integrated Project (ADC + USART + GPIO)
  ├── EXTI Line Edge Detectors      ├── I2C Bus Communication Drivers
  ├── ADC Conversion Concepts       └── FreeRTOS Kernel Integration
  ├── DMA Hardware Stream Routing
  └── Flashing & Memory Models

```

---

## Driver Development Methodology

```
Step 1: Consult Datasheet (Table 11)   --> Locate physical pin and AF mapping.
Step 2: Consult RM (Section 2.2)       --> Identify target bus and peripheral base address.
Step 3: Consult RCC Chapter            --> Enable peripheral bus clock gate.
Step 4: Consult Register Summary       --> Compute target register offset addresses.
Step 5: Consult Register Details       --> Configure control & status bitmasks.
Step 6: Review Functional Description  --> Verify edge cases, clear sequences, and flags.
Step 7: Implement Software Driver      --> Apply base address, mask operations, and volatile access.

```
