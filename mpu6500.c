#include "stm32f411xe.h"

/* These are required by the startup code and linker script.
 * SystemInit is called before main() by the startup assembly.
 * _init is called by __libc_init_array. Both are left empty
 * because we configure everything manually. */
void SystemInit(void) {}
void _init(void)      {}

/* ----------------------------------------------------------------
 * SysTick — 1ms tick counter
 * Built into the Cortex-M4 core.
 * DUI0553 Section 4.4 — SysTick registers (LOAD, VAL, CTRL)
 * ---------------------------------------------------------------- */
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void systick_init(void)
{
    /* LOAD: reload value = ticks per millisecond - 1
     * At 16 MHz HSI: 16,000,000 / 1000 - 1 = 15999
     * DUI0553 Section 4.4.2 */
    SysTick->LOAD = 15999UL;

    /* VAL: reset current count to 0 */
    SysTick->VAL  = 0;

    /* CTRL register — DUI0553 Section 4.4.2
     * bit 2 = CLKSOURCE : 1 = use processor (AHB) clock
     * bit 1 = TICKINT   : 1 = fire interrupt on reaching 0
     * bit 0 = ENABLE    : 1 = start the counter
     * Write 0x7 to set all three bits */
    SysTick->CTRL = 0x7UL;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms);
}

/* ----------------------------------------------------------------
 * USART1 — TX only on PA9, 115200 baud
 *
 * WHY PA9?
 *   PA13 = SWDIO, PA14 = SWCLK — those are your ST-Link debug
 *   lines, not a serial port. USART1_TX lives on PA9 at AF7.
 *   Connect PA9 → RX pin of your USB-serial adapter.
 *
 * PIN SETUP — RM0383 Section 8.4:
 *   GPIOA_MODER   bits [19:18] for PA9 → 10 = Alternate Function
 *   GPIOA_AFRH    bits [7:4]   for PA9 → 0111 = AF7 (USART1)
 *
 * BAUD RATE — RM0383 Table 75 (fPCLK=16MHz, oversampling by 16):
 *   115200 baud entry gives register value 8.6875
 *   mantissa = 8    → USART_BRR bits [15:4]
 *   fraction = 0.6875 × 16 = 11  → USART_BRR bits [3:0]
 *   BRR = (8 << 4) | 11 = 0x008B
 *   Actual baud = 115108 Hz, error = 0.08%
 *
 * KEY REGISTERS — RM0383 Section 19.6:
 *   USART1->BRR  — baud rate
 *   USART1->CR1  — bit 13 UE (enable), bit 3 TE (TX enable)
 *   USART1->SR   — bit 7 TXE (TX register empty), bit 6 TC (TX complete)
 *   USART1->DR   — write byte here to transmit
 *
 * CLOCK:
 *   USART1 is on APB2.
 *   RCC_APB2ENR bit 4 = USART1EN — RM0383 Section 6.3.12
 *   RCC_AHB1ENR bit 0 = GPIOAEN  — RM0383 Section 6.3.10
 * ---------------------------------------------------------------- */
static void usart1_init(void)
{
    /* 1. Enable GPIOA clock
     *    RCC_AHB1ENR bit 0 = GPIOAEN
     *    RM0383 Section 6.3.10 */
    RCC->AHB1ENR |= (1U << 0);

    /* 2. PA9 → Alternate Function mode
     *    GPIOA_MODER bits [19:18] = 10
     *    RM0383 Section 8.4.1
     *    First clear both bits, then write 10 */
    GPIOA->MODER &= ~(0x3U << 18);
    GPIOA->MODER |=  (0x2U << 18);

    /* 3. PA9 → AF7 (USART1_TX)
     *    PA9 is in AFRH (AFR[1]), occupies bits [7:4]
     *    AF7 = 0x7
     *    RM0383 Section 8.4.9 */
    GPIOA->AFR[1] &= ~(0xFU << 4);
    GPIOA->AFR[1] |=  (0x7U << 4);

    /* 4. Enable USART1 clock
     *    RCC_APB2ENR bit 4 = USART1EN
     *    RM0383 Section 6.3.12 */
    RCC->APB2ENR |= (1U << 4);

    /* 5. Baud rate = 115200 at 16 MHz
     *    USART_BRR = 0x008B (from RM0383 Table 75)
     *    bits [15:4] = mantissa = 8
     *    bits [3:0]  = fraction = 11 (0xB) */
    USART1->BRR = 0x008BU;

    /* 6. Enable USART and TX
     *    USART_CR1 bit 13 = UE  (USART enable)
     *    USART_CR1 bit 3  = TE  (transmitter enable)
     *    RM0383 Section 19.6.4 */
    USART1->CR1 = (1U << 13) | (1U << 3);
}

/* Send one byte.
 * Poll SR bit 7 (TXE) before writing — means TX register is empty.
 * RM0383 Section 19.6.1 */
static void usart1_send_byte(uint8_t b)
{
    while (!(USART1->SR & (1U << 7)));  /* wait: TXE = 1 */
    USART1->DR = b;
}

static void usart1_send_string(const char *s)
{
    while (*s)
        usart1_send_byte((uint8_t)*s++);
    /* Wait for TC (bit 6) — last byte fully shifted out */
    while (!(USART1->SR & (1U << 6)));
}

/* ----------------------------------------------------------------
 * Small integer print helpers — no printf needed
 * ---------------------------------------------------------------- */
static void send_int16(int16_t val)
{
    char buf[8];
    int  i = 0;
    if (val < 0) { usart1_send_byte('-'); val = -val; }
    if (val == 0) { usart1_send_byte('0'); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i > 0)   { usart1_send_byte((uint8_t)buf[--i]); }
}

/* ----------------------------------------------------------------
 * SPI2 — MPU-6500 on PB12/13/14/15
 *
 * PIN MAPPING (STM32F411CE datasheet Table 9):
 *   PB12 → manual GPIO output  (CS, active LOW)
 *   PB13 → SPI2_SCK  AF5
 *   PB14 → SPI2_MISO AF5  (connects to ADO on module)
 *   PB15 → SPI2_MOSI AF5  (connects to SDA on module)
 *
 * WHY manual CS on PB12?
 *   Hardware NSS can deassert between bytes in some configurations.
 *   A plain GPIO output gives you full control.
 *
 * SPI2 is on APB1 (16 MHz at default HSI).
 * MPU-6500 max SPI clock = 1 MHz for all registers
 * (MPU-6500 datasheet Section 3.3.3, Table 5).
 * APB1 16 MHz / 32 = 500 kHz — safely under 1 MHz.
 *
 * SPI_CR1 — RM0383 Section 20.5.1:
 *   bits [5:3] BR[2:0] = 100 → clock / 32 = 500 kHz
 *   bit  2     MSTR    = 1   → master mode
 *   bit  1     CPOL    = 0   → clock idle LOW   (Mode 0)
 *   bit  0     CPHA    = 0   → sample on rising  (Mode 0)
 *   bit  9     SSM     = 1   → software NSS management
 *   bit  8     SSI     = 1   → internal NSS = HIGH
 *                              (prevents mode-fault in master)
 *   bit  7     LSBFIRST= 0   → MSB first (MPU-6500 Section 6.5)
 *   bit  11    DFF     = 0   → 8-bit frame
 *   bit  6     SPE     = 1   → SPI enable (set last)
 *
 * CLOCK:
 *   RCC_APB1ENR bit 14 = SPI2EN — RM0383 Section 6.3.11
 *   RCC_AHB1ENR bit 1  = GPIOBEN — RM0383 Section 6.3.10
 * ---------------------------------------------------------------- */
static void spi2_init(void)
{
    /* 1. Enable GPIOB clock
     *    RCC_AHB1ENR bit 1 = GPIOBEN */
    RCC->AHB1ENR |= (1U << 1);

    /* 2. PB12 → output mode (manual CS)
     *    GPIOB_MODER bits [25:24] = 01 = output */
    GPIOB->MODER &= ~(0x3U << 24);
    GPIOB->MODER |=  (0x1U << 24);

    /* 3. PB13, PB14, PB15 → Alternate Function mode
     *    GPIOB_MODER bits [27:26] [29:28] [31:30] = 10
     *    RM0383 Section 8.4.1 */
    GPIOB->MODER &= ~((0x3U << 26) | (0x3U << 28) | (0x3U << 30));
    GPIOB->MODER |=   (0x2U << 26) | (0x2U << 28) | (0x2U << 30);

    /* 4. PB13, PB14, PB15 → AF5 (SPI2)
     *    All three are in AFRH (AFR[1])
     *    PB13 → bits [23:20], PB14 → bits [27:24], PB15 → bits [31:28]
     *    AF5 = 0x5
     *    RM0383 Section 8.4.9 */
    GPIOB->AFR[1] &= ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOB->AFR[1] |=   (0x5U << 20) | (0x5U << 24) | (0x5U << 28);

    /* 5. CS HIGH — deselect MPU-6500 */
    GPIOB->ODR |= (1U << 12);

    /* 6. Enable SPI2 clock
     *    RCC_APB1ENR bit 14 = SPI2EN
     *    RM0383 Section 6.3.11 */
    RCC->APB1ENR |= (1U << 14);

    /* 7. Configure SPI2_CR1
     *    Write all config bits together (SPE=0 still)
     *    bit 9  SSM     = 1
     *    bit 8  SSI     = 1
     *    bits 5:3 BR    = 100 (value 4 shifted left 3) → /32
     *    bit 2  MSTR    = 1
     *    bit 1  CPOL    = 0   (leave 0, Mode 0)
     *    bit 0  CPHA    = 0   (leave 0, Mode 0)
     *    RM0383 Section 20.5.1 */
    SPI2->CR1 = (1U << 9)        /* SSM  */
              | (1U << 8)        /* SSI  */
              | (0x4U << 3)      /* BR = 100 → /32 */
              | (1U << 2);       /* MSTR */

    /* 8. Enable SPI2 — set SPE (bit 6) last */
    SPI2->CR1 |= (1U << 6);
}

/* Exchange one byte over SPI2.
 * SPI_SR — RM0383 Section 20.5.3:
 *   bit 1 TXE  = 1 when TX buffer empty (ready to write)
 *   bit 0 RXNE = 1 when RX buffer has a byte ready to read
 *   bit 7 BSY  = 1 while transfer in progress */
static uint8_t spi2_transfer(uint8_t tx)
{
    while (!(SPI2->SR & (1U << 1)));   /* wait TXE = 1  */
    SPI2->DR = tx;                      /* load TX byte  */
    while (!(SPI2->SR & (1U << 0)));   /* wait RXNE = 1 */
    return (uint8_t)SPI2->DR;          /* read RX byte  */
}

/* ----------------------------------------------------------------
 * MPU-6500 register read / write
 *
 * Transaction format — MPU-6500 datasheet Section 6.5:
 *   First byte: bit 7 = R/W (1=read, 0=write), bits 6:0 = address
 * ---------------------------------------------------------------- */
static void mpu_write(uint8_t reg, uint8_t data)
{
    GPIOB->ODR &= ~(1U << 12);         /* CS LOW   */
    spi2_transfer(reg & 0x7F);         /* header: write, address */
    spi2_transfer(data);               /* data byte */
    while (SPI2->SR & (1U << 7));      /* wait BSY = 0 */
    GPIOB->ODR |=  (1U << 12);         /* CS HIGH  */
}

static uint8_t mpu_read(uint8_t reg)
{
    uint8_t val;
    GPIOB->ODR &= ~(1U << 12);
    spi2_transfer(reg | 0x80);         /* header: read, address */
    val = spi2_transfer(0xFF);         /* dummy byte → clocks in data */
    while (SPI2->SR & (1U << 7));
    GPIOB->ODR |=  (1U << 12);
    return val;
}

/* Burst read: one header, then clock in len bytes back-to-back.
 * MPU-6500 auto-increments register pointer while CS stays LOW.
 * MPU-6500 datasheet Section 6.5 */
static void mpu_read_burst(uint8_t start_reg, uint8_t *buf, uint8_t len)
{
    GPIOB->ODR &= ~(1U << 12);
    spi2_transfer(start_reg | 0x80);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = spi2_transfer(0xFF);
    while (SPI2->SR & (1U << 7));
    GPIOB->ODR |=  (1U << 12);
}

/* ----------------------------------------------------------------
 * MPU-6500 register addresses
 * Source: MPU-6500 Register Map and Register Descriptions.
 * PWR_MGMT_1 (0x6B) confirmed in datasheet Section 5.1.
 * USER_CTRL  (0x6A) confirmed in datasheet Section 6.1.
 * ACCEL_XOUT_H (0x3B) is the start of the 14-byte sensor block.
 * WHO_AM_I (0x75) returns 0x70 for MPU-6500.
 * ---------------------------------------------------------------- */
#define MPU_WHO_AM_I     0x75
#define MPU_PWR_MGMT_1   0x6B
#define MPU_USER_CTRL    0x6A
#define MPU_GYRO_CONFIG  0x1B
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_ACCEL_XOUT_H 0x3B

static void mpu_init(void)
{
    /* Power-up settling time.
     * Table 4 (MPU-6500 datasheet A.C. Electrical Characteristics):
     * start-up time from power-up = 11 ms typical, 100 ms max. */
    delay_ms(100);

    /* Wake device: clear SLEEP bit.
     * PWR_MGMT_1 = 0x00 → SLEEP=0, all other bits cleared.
     * MPU-6500 datasheet Section 5.1 */
    mpu_write(MPU_PWR_MGMT_1, 0x00);
    delay_ms(10);

    /* Disable I2C interface, force SPI-only.
     * USER_CTRL bit 4 = I2C_IF_DIS = 1.
     * Write 0x10 = 0b00010000.
     * MPU-6500 datasheet Section 6.1 */
    mpu_write(MPU_USER_CTRL, 0x10);

    /* Gyroscope full-scale range: FS_SEL bits [4:3] = 00 → ±250 °/s
     * Sensitivity = 131 LSB/(°/s) — Table 1, MPU-6500 datasheet */
    mpu_write(MPU_GYRO_CONFIG, 0x00);

    /* Accelerometer full-scale range: AFS_SEL bits [4:3] = 00 → ±2g
     * Sensitivity = 16384 LSB/g — Table 2, MPU-6500 datasheet */
    mpu_write(MPU_ACCEL_CONFIG, 0x00);
}

/* ----------------------------------------------------------------
 * MAIN
 * ---------------------------------------------------------------- */
int main(void)
{
    /* Enable GPIOC clock for PC13 LED
     *  RCC_AHB1ENR bit 2 = GPIOCEN — RM0383 Section 6.3.10 */
    RCC->AHB1ENR |= (1U << 2);

    /* PC13 → output mode
     *  GPIOC_MODER bits [27:26] = 01 */
    GPIOC->MODER &= ~(0x3U << 26);
    GPIOC->MODER |=  (0x1U << 26);

    systick_init();
    usart1_init();
    spi2_init();
    mpu_init();

    /* Sanity check — WHO_AM_I should return 0x70 for MPU-6500 */
    uint8_t who = mpu_read(MPU_WHO_AM_I);
    usart1_send_string("WHO_AM_I: 0x");
    usart1_send_byte("0123456789ABCDEF"[(who >> 4) & 0xF]);
    usart1_send_byte("0123456789ABCDEF"[(who     ) & 0xF]);
    usart1_send_string("\r\n");

    if (who != 0x70)
    {
        usart1_send_string("ERROR: unexpected WHO_AM_I. Check wiring.\r\n");
        while (1);
    }

    usart1_send_string("OK — sensor loop starting\r\n");
    usart1_send_string("AX,AY,AZ,GX,GY,GZ\r\n");

    uint8_t  raw[14];
    int16_t  ax, ay, az, gx, gy, gz;

    while (1)
    {
        /* Burst-read 14 bytes from ACCEL_XOUT_H (0x3B).
         * Layout: AX_H AX_L  AY_H AY_L  AZ_H AZ_L
         *         TEMP_H TEMP_L
         *         GX_H GX_L  GY_H GY_L  GZ_H GZ_L
         * MPU-6500 datasheet Section 4.13 */
        mpu_read_burst(MPU_ACCEL_XOUT_H, raw, 14);

        /* Reconstruct signed 16-bit values
         * High byte comes first — shift it up 8, OR in low byte */
        ax = (int16_t)((raw[0]  << 8) | raw[1]);
        ay = (int16_t)((raw[2]  << 8) | raw[3]);
        az = (int16_t)((raw[4]  << 8) | raw[5]);
        /* raw[6] and raw[7] = temperature, skipped */
        gx = (int16_t)((raw[8]  << 8) | raw[9]);
        gy = (int16_t)((raw[10] << 8) | raw[11]);
        gz = (int16_t)((raw[12] << 8) | raw[13]);

        /* Transmit as CSV */
        send_int16(ax); usart1_send_byte(',');
        send_int16(ay); usart1_send_byte(',');
        send_int16(az); usart1_send_byte(',');
        send_int16(gx); usart1_send_byte(',');
        send_int16(gy); usart1_send_byte(',');
        send_int16(gz);
        usart1_send_string("\r\n");

        /* Toggle LED — visual heartbeat */
        GPIOC->ODR ^= (1U << 13);

        delay_ms(100);
    }
}
