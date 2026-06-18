# STM32 Timers

> **Board:** STM32F411CEU6 Black Pill  
> **Reference:** DS10314 Rev 8 (Datasheet), RM0383 (Reference Manual)  
> **Default clock:** 16 MHz HSI (no PLL configured)

---

## What Are Timers?

A hardware timer is essentially a high-speed digital counter that ticks at a precise frequency. When that counter reaches a specific value — the moment the "time arrives" — the hardware can trigger several distinct actions completely on its own:

1. **Trigger an interrupt** — alerts the CPU
2. **Change a pin state** — toggle, pull high, or pull low
3. **Fire TRGO** — start an ADC or DAC conversion
4. **Signal DMA** — move data in memory automatically

We simply configure which action to take; the hardware handles the rest.

Timers can also function as stopwatches. They are capable of analysing signals, pulse widths, and PWM inputs. When a timer detects a rising or falling edge on a pin, it can:

1. **Capture the timestamp** — record exactly when the pulse arrived
2. **Count pulses** — keep a running tally of incoming edges
3. **React to the edge** — instantly reset the counter to zero, start a previously stopped timer, or pause the timer for the duration of the pulse

The configuration for signal input and capture will be covered in a later chapter.

---

## How Does a Timer Work?

A timer is a dedicated piece of hardware inside the microcontroller. It uses the MCU's clock as its timing source, receiving pulses through internal buses such as APB1 or APB2. The timer increments its **Counter Register (CNT)** by one on every incoming pulse.

Before reaching the counter, the main clock passes through a **Prescaler (PSC)**, which acts as a frequency divider. For example, if the MCU clock is running at 16 MHz and the prescaler value is set to 15, the clock is divided by 16 (= 1 + 15), dropping the timer's ticking frequency down to a clean 1 MHz.

Most general-purpose STM32 timers feature **four independent channels**, each mapped to a physical GPIO pin, allowing the timer to interact directly with the outside world. Depending on your configuration, the timer can count up or count down. It counts until it hits a specific target value stored in the **Auto-Reload Register (ARR)**, at which point it resets back to zero and repeats the cycle.

> **Remember:** The counter (CNT) is constantly running in the background — ticking from 0 up to the ARR value, resetting, and repeating — without any CPU involvement.

There is one more key register: the **Capture/Compare Register (CCR)**. Inside each timer channel, a hardware comparator continuously asks: *"Is the current count (CNT) less than my target threshold (CCR)?"*

1. **While CNT < CCR** — the timer hardware pulls the channel pin **HIGH**
2. **The moment CNT ≥ CCR** — the hardware instantly drops the pin **LOW**
3. **When CNT reaches ARR** — the counter resets to 0 and the pin is pulled **HIGH** again, starting the next cycle

This is the mechanism that generates a PWM waveform. The two governing formulas are:

```
freq = timer_clock / (PSC + 1) / (ARR + 1)
CCR  = duty_fraction × (ARR + 1)
```

To change the duty cycle dynamically at runtime, rewrite the CCR register value. However, you must also **enable the Output Compare Preload bit (OCxPE)**. This acts as a safety buffer: when you update the duty cycle in code, the preload bit forces the hardware to wait until the current counting cycle safely completes before applying the new value, preventing waveform glitches.

---

## Configuring a Timer for PWM Output

Now that the internal mechanism is clear, the next task is wiring it all together in hardware. There are three distinct layers to configure: the **clock system (RCC)**, the **GPIO pin**, and the **timer itself**. Each layer has its own set of registers.

### Layer 1 — Enable the Peripheral Clocks (RCC)

> ⚠️ **This is always the first step.** A peripheral that has not been given a clock is completely deaf — writing to any of its registers has no effect whatsoever.

The **Reset and Clock Control (RCC)** block distributes the system clock to every peripheral. By default, peripherals are clock-gated off to save power.

**`RCC_AHB1ENR`** — enables the clock supply to GPIO ports.
- Bit 0 → GPIOAEN (Port A)
- Bit 1 → GPIOBEN (Port B)
- Bit 2 → GPIOCEN (Port C)

**`RCC_APB1ENR`** — enables the clock supply to general-purpose timers on the APB1 bus.  
On the STM32F411, TIM2, TIM3, TIM4, and TIM5 all live here.
- Bit 0 → TIM2EN
- Bit 1 → TIM3EN
- Bit 2 → TIM4EN
- Bit 3 → TIM5EN

**`RCC_APB2ENR`** — enables the clock supply to TIM1 (advanced-control timer, APB2 bus).
- Bit 0 → TIM1EN

> On the STM32F411, both APB1 and APB2 timers run at up to 100 MHz. At the default 16 MHz HSI with no PLL, both buses deliver 16 MHz to their timers.

---

### Layer 2 — Configure the GPIO Pin

For a pin to carry a PWM signal, it must be switched out of its default I/O mode and handed over to the timer peripheral. This involves two registers.

**`GPIOx_MODER`** — sets the operating mode of each pin (2 bits per pin).

| Bits | Mode |
|------|------|
| `00` | Input (default after reset) |
| `01` | General-purpose output |
| `10` | **Alternate Function ← required for PWM** |
| `11` | Analog |

Pin N occupies bits `[2N+1 : 2N]`. To configure PB4, write `0b10` into bits `[9:8]`.

**`GPIOx_AFRL`** — selects which alternate function (which peripheral) connects to pins 0–7. Each pin gets a 4-bit field, holding a value from AF0 to AF15.

**`GPIOx_AFRH`** — same as AFRL, but for pins 8–15.

The correct AF number for each pin-timer combination is found in **Table 9 of the STM32F411xC/xE datasheet (DS10314 Rev 8), starting at page 48**. Common PWM-capable mappings for the UFQFPN48 package (Black Pill):

| Pin | AF Number | Timer Channel |
|-----|-----------|---------------|
| PA0 | AF1 | TIM2_CH1 |
| PA1 | AF1 | TIM2_CH2 |
| PA2 | AF1 | TIM2_CH3 |
| PA3 | AF1 | TIM2_CH4 |
| PA5 | AF1 | TIM2_CH1 |
| **PB4** | **AF2** | **TIM3_CH1** |
| **PB5** | **AF2** | **TIM3_CH2** |
| PB6 | AF2 | TIM4_CH1 |
| PB7 | AF2 | TIM4_CH2 |
| PB8 | AF2 | TIM4_CH3 |
| PB9 | AF2 | TIM4_CH4 |
| PA8 | AF1 | TIM1_CH1 |
| PA9 | AF1 | TIM1_CH2 |

> These mappings are fixed in silicon. Always verify against **Table 9** in the DS10314 datasheet before writing any code.

---

### Layer 3 — Configure the Timer Registers

With the clocks enabled and the pin configured, the timer itself needs to be set up.

**`TIMx_PSC`** — the prescaler register. Divides the input clock before it reaches the counter.
```
timer_clock = input_clock / (PSC + 1)
```

**`TIMx_ARR`** — the auto-reload register. The counter ceiling. Sets the PWM period and frequency. This value is **shared across all four channels** of the same timer.
```
PWM_frequency = timer_clock / (ARR + 1)
```

**`TIMx_CCR1` / `TIMx_CCR2` / `TIMx_CCR3` / `TIMx_CCR4`** — the capture/compare register for each channel. Sets the duty cycle independently per channel. Can be written at any time while the timer runs.
```
CCR = duty_fraction × (ARR + 1)
```

**`TIMx_CCMR1`** — configures the output compare mode for CH1 and CH2.
- `OC1M` (bits [6:4]) → set to `0b110` for PWM Mode 1 on CH1
- `OC1PE` (bit [3]) → set to `1` to enable preload for CH1
- `OC2M` (bits [14:12]) → set to `0b110` for PWM Mode 1 on CH2
- `OC2PE` (bit [11]) → set to `1` to enable preload for CH2

**`TIMx_CCMR2`** — same structure as CCMR1, but governs CH3 and CH4.
- `OC3M` / `OC3PE` → CH3
- `OC4M` / `OC4PE` → CH4

> When modifying CCMR1 or CCMR2, always use a **read-modify-write** operation. Writing a raw value can silently zero out the configuration of the other channel sharing the same register.

**`TIMx_CCER`** — the capture/compare enable register. The final gate between the timer's output logic and the physical pin.
- Bit 0 → `CC1E` — enable CH1 output
- Bit 4 → `CC2E` — enable CH2 output
- Bit 8 → `CC3E` — enable CH3 output
- Bit 12 → `CC4E` — enable CH4 output

**`TIMx_CR1`** — the primary timer control register.
- Bit 0 → `CEN` — starts the counter. **Set this last.**
- Bit 7 → `ARPE` — enables ARR preload buffering. Strongly recommended.

**`TIMx_EGR`** — the event generation register.
- Bit 0 → `UG` — writing `1` forces an immediate update event, transferring the PSC and ARR preload values into the active registers and resetting the counter. Write this once after setting PSC and ARR.

**`TIMx_BDTR`** *(TIM1 only)* — the break and dead-time register.
- Bit 15 → `MOE` (Main Output Enable) — must be set to `1` for TIM1 outputs to become active. General-purpose timers (TIM2–TIM5) do not have this register and do not need this step.

---

## Step-by-Step: PWM on PB4 and PB5 (STM32F411CEU6)

**Goal:** Two independent PWM outputs on the same timer.
- **PB4** → TIM3 CH1 → 1 kHz → 25% duty cycle
- **PB5** → TIM3 CH2 → 1 kHz → 75% duty cycle

**Clock:** 16 MHz HSI (default, no PLL — same as the blink example)  
**Timer clock after PSC:** 1 MHz (PSC = 15)  
**Period:** 1 ms → ARR = 999

---

### The Configuration Walkthrough

**Step 1 — Enable the GPIO Port B clock**

Set bit 1 (`GPIOBEN`) in `RCC_AHB1ENR`. Port B is now live.  
Without this, every register write to GPIOB is silently discarded.

**Step 2 — Enable the TIM3 clock**

Set bit 1 (`TIM3EN`) in `RCC_APB1ENR`. TIM3 is now live.

**Step 3 — Set PB4 and PB5 to Alternate Function mode**

In `GPIOB_MODER`:
- PB4 occupies bits [9:8] → write `0b10`
- PB5 occupies bits [11:10] → write `0b10`

Both pins are now disconnected from the CPU and listening to the alternate function path.

**Step 4 — Select AF2 for PB4 and PB5**

Both pins are in the 0–7 range, so use `GPIOB_AFRL`:
- PB4's 4-bit field → bits [19:16] → write `2`
- PB5's 4-bit field → bits [23:20] → write `2`

The internal mux connects PB4 to TIM3 CH1 and PB5 to TIM3 CH2.

**Step 5 — Set the prescaler**

Write `15` to `TIM3_PSC`.  
Result: 16 MHz / (15 + 1) = **1 MHz** timer clock (1 tick per µs).

**Step 6 — Set the period and enable ARR preload**

Write `999` to `TIM3_ARR`.  
Result: 1 MHz / (999 + 1) = **1 kHz** PWM frequency.  
Also set bit 7 (`ARPE`) in `TIM3_CR1` now.

**Step 7 — Force-load the buffers**

Write `1` to bit 0 (`UG`) in `TIM3_EGR`.  
PSC and ARR are now active in hardware; the counter is reset to 0.

**Step 8 — Set the duty cycles**

Write `250` to `TIM3_CCR1` → 250 / 1000 = **25% duty** on PB4.  
Write `750` to `TIM3_CCR2` → 750 / 1000 = **75% duty** on PB5.

**Step 9 — Configure PWM mode for both channels**

In `TIM3_CCMR1` (use read-modify-write):
- `OC1M` (bits [6:4]) → `0b110` — PWM Mode 1 for CH1
- `OC1PE` (bit [3]) → `1` — preload enable for CH1
- `OC2M` (bits [14:12]) → `0b110` — PWM Mode 1 for CH2
- `OC2PE` (bit [11]) → `1` — preload enable for CH2

**Step 10 — Enable both channel outputs**

In `TIM3_CCER`:
- Set bit 0 (`CC1E`) — connect CH1 comparator output to PB4
- Set bit 4 (`CC2E`) — connect CH2 comparator output to PB5

**Step 11 — Start the timer**

Set bit 0 (`CEN`) in `TIM3_CR1`.

Both pins are now toggling at 1 kHz, driven entirely in hardware. The CPU is free.

---

### The Example Code

This follows the same style as the blink example — bare-metal, no HAL, direct register access with CMSIS headers.

```c
#include "stm32f411xe.h"

void SystemInit(void) {}
void _init(void)      {}

/*
 * PWM on PB4 (TIM3_CH1) and PB5 (TIM3_CH2)
 *
 * Source: DS10314 Rev 8, Table 9, page 48
 *   PB4 → AF2 → TIM3_CH1
 *   PB5 → AF2 → TIM3_CH2
 *
 * Clock: 16 MHz HSI (default, no PLL)
 * PSC = 15  → timer_clock = 16 MHz / 16 = 1 MHz
 * ARR = 999 → PWM freq    = 1 MHz / 1000 = 1 kHz
 *
 * CCR1 = 250 → PB4 duty = 250/1000 = 25%
 * CCR2 = 750 → PB5 duty = 750/1000 = 75%
 */

int main(void)
{
    /* ---- Step 1: Enable GPIO Port B clock -------------------------
     * RM0383 Section 6.3.10 — RCC_AHB1ENR, bit 1 = GPIOBEN
     * Without this, all GPIOB register writes are silently ignored. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* ---- Step 2: Enable TIM3 clock --------------------------------
     * RM0383 Section 6.3.12 — RCC_APB1ENR, bit 1 = TIM3EN
     * TIM3 is on the APB1 bus on the STM32F411. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* ---- Steps 3 & 4: Configure PB4 and PB5 for Alternate Function
     * RM0383 Section 8.4.1 — GPIOx_MODER, 2 bits per pin
     *   PB4 → bits [9:8]  = 0b10 (AF mode)
     *   PB5 → bits [11:10] = 0b10 (AF mode)
     *
     * RM0383 Section 8.4.9 — GPIOx_AFRL, 4 bits per pin (pins 0-7)
     *   PB4 → bits [19:16] = 2 (AF2 = TIM3_CH1)
     *   PB5 → bits [23:20] = 2 (AF2 = TIM3_CH2)
     * Source: DS10314 Rev 8, Table 9, page 48               */

    /* Set PB4 and PB5 to Alternate Function mode */
    GPIOB->MODER &= ~( GPIO_MODER_MODER4 | GPIO_MODER_MODER5 );
    GPIOB->MODER |=  ( GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 );

    /* Select AF2 for PB4 and PB5 (both in AFRL — pins 0 to 7) */
    GPIOB->AFR[0] &= ~( GPIO_AFRL_AFSEL4 | GPIO_AFRL_AFSEL5 );
    GPIOB->AFR[0] |=  ( (2UL << GPIO_AFRL_AFSEL4_Pos) |
                        (2UL << GPIO_AFRL_AFSEL5_Pos) );

    /* ---- Steps 5 & 6: Set prescaler, period, and ARR preload -----
     * RM0383 Section 18.4.11 — TIMx_PSC
     * RM0383 Section 18.4.12 — TIMx_ARR
     * RM0383 Section 18.4.1  — TIMx_CR1, bit 7 = ARPE             */

    TIM3->PSC  = 15;     /* 16 MHz / (15+1) = 1 MHz timer clock     */
    TIM3->ARR  = 999;    /* 1 MHz  / (999+1) = 1 kHz PWM frequency  */
    TIM3->CR1 |= TIM_CR1_ARPE;   /* Enable ARR preload buffering     */

    /* ---- Step 7: Force-load PSC and ARR into active registers -----
     * RM0383 Section 18.4.6 — TIMx_EGR, bit 0 = UG (Update Generation)
     * Without this, PSC and ARR won't apply until the first overflow. */
    TIM3->EGR  = TIM_EGR_UG;

    /* ---- Step 8: Set duty cycles ----------------------------------
     * RM0383 Section 18.4.14 — TIMx_CCR1
     * RM0383 Section 18.4.15 — TIMx_CCR2                          */
    TIM3->CCR1 = 250;    /* PB4: 250/1000 = 25% duty cycle          */
    TIM3->CCR2 = 750;    /* PB5: 750/1000 = 75% duty cycle          */

    /* ---- Step 9: Configure PWM Mode 1 for CH1 and CH2 ------------
     * RM0383 Section 18.4.7 — TIMx_CCMR1
     *   OC1M  bits [6:4]   = 0b110 → PWM Mode 1, CH1
     *   OC1PE bit  [3]     = 1     → preload enable, CH1
     *   OC2M  bits [14:12] = 0b110 → PWM Mode 1, CH2
     *   OC2PE bit  [11]    = 1     → preload enable, CH2
     * Use |= to preserve any other bits in the register.            */
    TIM3->CCMR1 |= ( TIM_CCMR1_OC1PE |
                     (0b110UL << TIM_CCMR1_OC1M_Pos) |
                     TIM_CCMR1_OC2PE |
                     (0b110UL << TIM_CCMR1_OC2M_Pos) );

    /* ---- Step 10: Enable channel outputs --------------------------
     * RM0383 Section 18.4.9 — TIMx_CCER
     *   CC1E bit [0] = 1 → connect CH1 comparator output to PB4
     *   CC2E bit [4] = 1 → connect CH2 comparator output to PB5    */
    TIM3->CCER |= ( TIM_CCER_CC1E | TIM_CCER_CC2E );

    /* ---- Step 11: Start the counter — do this last ----------------
     * RM0383 Section 18.4.1 — TIMx_CR1, bit 0 = CEN               */
    TIM3->CR1 |= TIM_CR1_CEN;

    /*
     * PB4 and PB5 are now producing 1 kHz PWM in hardware.
     * The CPU does nothing for this — the timer runs autonomously.
     *
     * To change duty cycle at runtime, just write a new CCR value:
     *   TIM3->CCR1 = 500;   // PB4 → 50%
     *   TIM3->CCR2 = 100;   // PB5 → 10%
     * The preload buffer applies the change at the next period.
     */
    while (1)
    {
        /* Your application code goes here.
         * The PWM continues running without any CPU involvement. */
    }
}
```

---

## Adding More PWM Outputs

### Same Timer — Different Duty Cycle

All channels on one timer share the same frequency (ARR). Adding another channel only requires configuring the GPIO and the new CCR — the timer is already running.

**Example: add PB6 at 50% duty (TIM4 CH1, AF2)**  
This needs TIM4 because PB6 is wired to TIM4, not TIM3.  
Run the full 11-step sequence again for TIM4 with PSC = 15, ARR = 999, CCR1 = 500.

### Different Frequencies — A Different Timer

Each timer has exactly one frequency (one ARR value). To run a second frequency simultaneously, use a second timer. Both run independently and neither affects the other.

---

## Changing Duty Cycle at Runtime

Write a new value to the relevant `TIMx_CCRx` register at any point during execution. Because preload (`OC1PE`) is enabled, the hardware buffers the new value and applies it cleanly at the start of the next PWM period — no glitches, no need to stop the timer.

```c
/* Change PB4 to 50% duty */
TIM3->CCR1 = 500;

/* Change PB5 to 10% duty */
TIM3->CCR2 = 100;

/* Servo on a 50 Hz timer (ARR = 19999, PSC = 15) */
TIM2->CCR1 = 1000;   /* 1.0 ms pulse → far left   */
TIM2->CCR1 = 1500;   /* 1.5 ms pulse → centre     */
TIM2->CCR1 = 2000;   /* 2.0 ms pulse → far right  */
```

Writing to both CCRs in sequence loads both preload buffers. They apply simultaneously at the next counter reset.

---

## Quick Reference

### Configuration Checklist

```
BEFORE YOU START — look up Table 9, DS10314 Rev 8, page 48
─────────────────────────────────────────────────────────────
□ Pin → Timer → Channel → AF number

CALCULATE
─────────────────────────────────────────────────────────────
□ PSC  = (input_clock / target_timer_clock) - 1
□ ARR  = (target_timer_clock / PWM_freq) - 1
□ CCR  = duty_fraction × (ARR + 1)

CONFIGURATION SEQUENCE — order matters
─────────────────────────────────────────────────────────────
□  1.  RCC_AHB1ENR   → enable GPIO port clock
□  2.  RCC_APB1ENR   → enable timer clock
         (use RCC_APB2ENR for TIM1)
□  3.  GPIOx_MODER   → set pin to AF mode (0b10)
□  4.  GPIOx_AFRL/H  → write AF number from Table 9
□  5.  TIMx_PSC      → set prescaler
□  6.  TIMx_ARR      → set period
□  6a. TIMx_CR1      → set ARPE = 1 (ARR preload)
□  7.  TIMx_EGR      → write UG = 1 (force-load buffers)
□  8.  TIMx_CCRx     → set duty cycle
□  9.  TIMx_CCMRx    → set OCxM = 0b110, OCxPE = 1
□ 10.  TIMx_CCER     → set CCxE = 1 (enable output)
□ 11.  TIMx_CR1      → set CEN = 1  ← start last
□ 12.  TIMx_BDTR     → set MOE = 1  ← TIM1 only
```

### Common Mistakes

| Symptom | Cause |
|---------|-------|
| Pin produces no signal | GPIO or timer clock not enabled in RCC |
| Wrong frequency | Check the actual timer input clock — at 16 MHz HSI, PSC = 15 for 1 MHz, not 83 |
| TIM1 outputs nothing despite correct configuration | `MOE` bit not set in `TIM1_BDTR` |
| Duty cycle glitches when updated at runtime | `OCxPE` not enabled in CCMR — turn on output compare preload |
| Enabling a second channel broke the first | Raw write to CCMR zeroed the other channel — always use `\|=` not `=` |
| PSC / ARR changes have no effect | `UG = 1` not written to `TIMx_EGR` after changing PSC or ARR |

---

*The next chapter covers Input Capture — using the timer to measure the frequency and pulse width of an incoming signal.*
