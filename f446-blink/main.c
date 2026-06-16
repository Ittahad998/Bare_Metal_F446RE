#include "stm32f446xx.h"

void SystemInit(void) {}
void _init(void) {}        /* ← fixes __libc_init_array */

static void delay(volatile uint32_t count)
{
    while (count--);
}

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODER5;
    GPIOA->MODER |=  GPIO_MODER_MODER5_0;

    while (1)
    {
        GPIOA->ODR |=  (1U << 5);
        delay(500000);
        GPIOA->ODR &= ~(1U << 5);
        delay(500000);
    }
}