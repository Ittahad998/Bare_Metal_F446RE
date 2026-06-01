# STM32F446RE Bare-Metal from Scratch

## What is Memory Mapped I/O

The STM32F446RE has a 4GB address space (0x00000000 to 0xFFFFFFFF).
The chip designers permanently assigned hardware peripherals to fixed
address ranges within this space.

| Region     | Start        | End          |
|------------|--------------|--------------|
| Flash      | 0x08000000   | 0x0807FFFF   |
| SRAM1      | 0x20000000   | 0x2001FFFF   |
| Peripherals| 0x40000000   | 0x5FFFFFFF   |

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
