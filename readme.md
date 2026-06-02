# STM32F446RE Bare-Metal from Scratch

## What is Memory Mapped I/O

The STM32F446RE has a 4GB address space (0x00000000 to 0xFFFFFFFF).
The chip designers permanently assigned hardware peripherals to fixed
address ranges within this space.

| Region      | Start      | End        |
|-------------|------------|------------|
| Flash       | 0x08000000 | 0x0807FFFF |
| SRAM1       | 0x20000000 | 0x2001FFFF |
| Peripherals | 0x40000000 | 0x5FFFFFFF |

To talk to hardware, we create a C pointer to the exact address of
that hardware register and read or write through it.

## Why volatile is Mandatory

Without volatile, the compiler optimizes away repeated writes to the
same address — it cannot see that hardware state changed externally.
volatile forces every read and write to happen exactly as written,
in exact order, every time.

## The Struct Overlay Pattern

Instead of raw pointer arithmetic, we define a C struct whose fields
mirror the peripheral register map exactly. The struct is then cast
to the peripheral base address.

This turns raw address manipulation into clean, readable code:

    GPIOA->MODER |= (1U << 10);

instead of:

    *((volatile uint32_t *)(0x40020000 + 0x00)) |= (1U << 10);

## Enabling Peripheral Clocks — RCC_AHB1ENR

Every peripheral on the STM32F446RE is dead until its clock is
enabled through the Reset and Clock Control (RCC) block.

GPIO ports live on the AHB1 bus. Their clocks are controlled through
the RCC_AHB1ENR register at offset 0x30 from RCC base (0x40023800).

Each bit in RCC_AHB1ENR enables one GPIO port:

| Bit | Peripheral |
|-----|------------|
| 0   | GPIOA      |
| 1   | GPIOB      |
| 2   | GPIOC      |
| 3   | GPIOD      |

### Why Always OR, Never Assign

When enabling a peripheral clock, always use the OR-assign operator:
```C
    RCC->AHB1ENR |= (1U << 0);  // enable GPIOA, touch nothing else
```
Never use plain assignment:
```C
    RCC->AHB1ENR = (1U << 0);  // WRONG
```
Plain assignment overwrites every bit in the register with 0 except
the one you intended to set. This silently disables the clocks of
every other peripheral that was already enabled, causing the MCU to
malfunction in ways that are extremely difficult to debug.

The OR-assign reads the current register value, sets only your
target bit, and writes the result back — leaving all other bits
completely untouched.

### Q: Why does plain assignment break other peripherals?
### A: Every bit in RCC_AHB1ENR controls one peripheral clock.
### A plain assignment forces all unspecified bits to 0, disabling
### every peripheral whose clock was previously enabled. The MCU
### loses those peripherals silently with no error or warning.
