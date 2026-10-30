#include "stm32f411xe.h"

void SystemInit(void) {}
void _init(void)      {}

/* ----------------------------------------------------------------
 * SysTick millisecond counter
 * SysTick is a 24-bit countdown timer built into Cortex-M4 core
 * DUI0553 Section 4.4 — SysTick registers
 * ---------------------------------------------------------------- */
static volatile uint32_t ms_ticks = 0;

/* SysTick interrupt fires every 1ms */
void SysTick_Handler(void)
{
    ms_ticks++;
}

/* Initialize SysTick for 1ms interrupt
 *
 * WHAT: Configures SysTick to count down from RELOAD to 0
 *       and fire an interrupt every 1ms
 * WHY:  RM0383 Section 4.4.1 — SysTick is driven by AHB clock
 *       F411 default clock = 16MHz HSI
 *       16,000,000 / 1000 = 16000 ticks per millisecond
 * HOW:  Every 16000 ticks = 1ms → interrupt → ms_ticks++
 */
static void systick_init(void)
{
    /* RELOAD value = (CPU_FREQ / 1000) - 1
     * At 16MHz HSI: 16000000/1000 - 1 = 15999 */
    SysTick->LOAD = 16000000UL / 1000UL - 1;

    /* Reset counter to 0 */
    SysTick->VAL = 0;

    /* WHAT: Sets bits [2:1:0] of CTRL register
     * bit 2 = CLKSOURCE: 1 = use processor clock (AHB)
     * bit 1 = TICKINT:   1 = enable interrupt on countdown
     * bit 0 = ENABLE:    1 = enable SysTick counter
     * WHY:  DUI0553 Section 4.4.2 — SysTick Control Register */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk    |
                    SysTick_CTRL_ENABLE_Msk;
}

/* Block for given number of milliseconds */
static void delay_ms(uint32_t ms)
{
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

/* ----------------------------------------------------------------
 * MAIN
 * ---------------------------------------------------------------- */
int main(void)
{
    /* Enable GPIOC clock — RM0383 Section 6.3.10, bit 2 = GPIOCEN */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* Set PC13 as output — RM0383 Section 8.4.1, bits [27:26] = 01 */
    GPIOC->MODER &= ~GPIO_MODER_MODER13;
    GPIOC->MODER |=  GPIO_MODER_MODER13_0;

    /* Start SysTick */
    systick_init();

    while (1)
    {
        GPIOC->ODR &= ~(1U << 13);  /* PC13 LOW  → LED ON  */
        delay_ms(50);               /* exactly 500ms        */

        GPIOC->ODR |=  (1U << 13);  /* PC13 HIGH → LED OFF */
        delay_ms(500);               /* exactly 500ms        */
    }
}