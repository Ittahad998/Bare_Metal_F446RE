
COMPLETE LEARNING SUMMARY
STM32F446RE Bare-Metal Embedded Systems
From Zero to Hardware Driver Understanding

Board  : STM32 Nucleo-64 (NUCLEO-F446RE)
Chip   : STM32F446RE (ARM Cortex-M4, up to 180MHz, 512KB Flash, 128KB SRAM)
Clock  : 16MHz HSI internal oscillator (default at reset)
Tools  : STM32CubeIDE, arm-none-eabi-gcc, RM0390 Reference Manual,
         STM32F446RE Datasheet, DUI0553 Cortex-M4 User Guide


THE FOUNDATIONAL SKILL YOU BUILT


Before writing any code, you learned to navigate three documents:

  STM32F446RE Datasheet
    Used for: physical pin locations, alternate function mapping,
    package diagrams, ADC channel to pin mapping.
    Key table: Table 11 - Pin and ball definitions.

  RM0390 Reference Manual
    Used for: everything about registers.
    Section 2.2  : Memory map - base addresses and bus assignments
    Section 6.3  : RCC - clock enable registers
    Section 7.4  : GPIO registers
    Section 10.3 : EXTI registers
    Section 13   : ADC registers
    Section 17.4 : Timer registers
    Section 25.6 : USART registers
    Chapter 8    : SYSCFG registers
    Chapter 9    : DMA registers

  DUI0553 Cortex-M4 User Guide
    Used for: ARM core peripherals not documented by ST.
    Section 4.2  : NVIC registers
    Section 4.4  : SysTick registers
    Section 2.3  : Vector table layout

The navigation path is always:
  Memory map (Section 2.2) to find base address and bus
  RCC chapter to find clock enable bit
  Peripheral chapter register summary to find offsets
  Individual register page to find bit meanings
  Functional description (earlier in same chapter) to understand behavior


================================================================================
LESSON 1 - GPIO OUTPUT
================================================================================

CONCEPT
  GPIO pins are memory-mapped hardware registers.
  Writing to a register physically changes voltage on a chip pin.
  Every peripheral is frozen by default - clock must be enabled first.

KEY REGISTERS
  RCC_AHB1ENR   Enable clock to GPIO port (bit 0=A, 1=B, 2=C...)
  GPIOx_MODER   Set pin mode: 00=input 01=output 10=AF 11=analog
  GPIOx_ODR     Write 1 or 0 to drive pin HIGH or LOW
  GPIOx_OTYPER  Output type: 0=push-pull 1=open-drain

WHAT YOU BUILT
  LED blink on PA5 using raw register addresses.
  RCC_AHB1ENR |= (1U << 0)       enable GPIOA clock
  GPIOA_MODER &= ~(3U << 10)     clear PA5 mode bits
  GPIOA_MODER |=  (1U << 10)     set PA5 as output
  GPIOA_ODR   |=  (1U << 5)      LED ON
  GPIOA_ODR   &= ~(1U << 5)      LED OFF

KEY CONCEPTS LEARNED
  Memory-mapped I/O: hardware registers have RAM-style addresses
  volatile keyword: prevents compiler from optimizing away register accesses
  Bitwise clear then set pattern: always &= ~mask before |= value
  Clock gating: peripheral is dead until its clock is enabled in RCC


================================================================================
LESSON 2 - GPIO INPUT
================================================================================

CONCEPT
  Reading a pin state means reading IDR - the input data register.
  Floating pins read noise - pull-up or pull-down resistor needed.
  PC13 on Nucleo has external pull-up, reads HIGH when button not pressed,
  LOW when pressed (falling edge on button press).

KEY REGISTERS
  GPIOx_IDR     Read-only. Bit N reflects current voltage on pin N.
  GPIOx_PUPDR   Pull resistor: 00=none 01=pull-up 10=pull-down (2 bits per pin)
  GPIOx_MODER   Set to 00 for input mode

WHAT YOU BUILT
  Button on PC13 controls LED on PA5.
  if (!(GPIOC_IDR & (1U << 13))) - button pressed, LED on
  else - button not pressed, LED off

KEY CONCEPTS LEARNED
  R/W column in register tables: IDR is read-only, writing has no effect
  External vs internal pull resistors
  Polling: CPU checks pin state continuously in a loop
  Polling limitation: CPU cannot do other work while checking


================================================================================
LESSON 3 - SYSTICK TIMER
================================================================================

CONCEPT
  SysTick is a 24-bit countdown timer built into the ARM Cortex-M4 core.
  Not an ST peripheral - documented in DUI0553, not RM0390.
  Counts down from a reload value to zero, fires interrupt, reloads, repeats.
  Used to generate accurate millisecond timebase.

KEY REGISTERS (addresses fixed by ARM for all Cortex-M chips)
  SYST_CSR    0xE000E010   Control: ENABLE(bit0) TICKINT(bit1) CLKSOURCE(bit2)
  SYST_RVR    0xE000E014   Reload value (write N-1 for N tick period)
  SYST_CVR    0xE000E018   Current value (write any value to clear to zero)

CONFIGURATION FOR 1MS AT 16MHZ
  SYST_RVR = 15999         16000 ticks per ms, minus 1
  SYST_CVR = 0             clear counter
  SYST_CSR = 0b111         processor clock + interrupt + enable

WHAT YOU BUILT
  volatile uint32_t ms_ticks = 0
  void SysTick_Handler(void) { ms_ticks++; }
  void delay_ms(uint32_t ms) { uint32_t start=ms_ticks; while((ms_ticks-start)<ms); }

KEY CONCEPTS LEARNED
  Interrupt: hardware calls a function automatically when event occurs
  Vector table: array of function pointers at 0x00000000 in Flash
  Weak symbols: startup file defines default handlers, your code overrides them
  SysTick_Handler name is not arbitrary - matches startup file weak symbol
  volatile on ms_ticks: written by interrupt, read by main, compiler cannot cache
  Startup file: runs before main(), sets up stack, copies variables, builds vector table


================================================================================
LESSON 4 - USART SERIAL COMMUNICATION
================================================================================

CONCEPT
  USART is dedicated silicon - a hardware block that handles serial communication.
  Not bit-banging. CPU writes one byte, hardware sends all bits automatically.
  PA2=USART2_TX, PA3=USART2_RX connected to ST-LINK, appears as virtual COM port.
  UART protocol: asynchronous, both sides agree on baud rate in advance.
  Frame: 1 start bit + 8 data bits + 1 stop bit = 8N1 format.

PIN MULTIPLEXING - THE CRITICAL CONCEPT
  Every GPIO pin connects internally to multiple peripherals via a hardware switch.
  MODER = 0b10 disconnects GPIO logic, connects pin to AF multiplexer.
  AFRL/AFRH selects which peripheral (AF0-AF15) actually connects to the pin.
  Both registers required. Neither alone is sufficient.
  PA2 with AF7 = USART2_TX (from datasheet Table 11)
  PA3 with AF7 = USART2_RX (from datasheet Table 11)

KEY REGISTERS
  RCC_APB1ENR   bit 17 = USART2EN (USART2 is on APB1 bus)
  GPIOx_MODER   bits [2n+1:2n] = 0b10 for alternate function mode
  GPIOx_AFRL    4 bits per pin (pins 0-7), write AF number
  GPIOx_AFRH    4 bits per pin (pins 8-15), write AF number
  USART_BRR     baud rate = peripheral_clock / (16 x baud_rate)
  USART_CR1     UE(bit13) TE(bit3) RE(bit2) M(bit12) PCE(bit10)
  USART_SR      TXE(bit7)=TX buffer empty, RXNE(bit5)=RX data ready
  USART_DR      write to transmit, read to receive

BAUD RATE MATH
  BRR = clock / (16 x baud)
  115200 at 16MHz: 16000000/115200 = 138.88
  Mantissa = 8 (bits [15:4]), Fraction = 11 (bits [3:0]) -> BRR = 0x008B
  9600 at 16MHz: mantissa=104 (0x68), fraction=3 (0x3) -> BRR = 0x0683

TRANSMIT PIPELINE
  TDR buffer -> Shift register -> PA2 pin -> ST-LINK -> USB -> PC
  TXE flag = 1 means TDR is empty, safe to write next byte
  Always check TXE before writing to DR or previous byte is lost silently

WHAT YOU BUILT
  USART2_Init(), USART2_SendChar(), USART2_SendString(), USART2_Print()
  printf-style print with %d %s %c %f support using va_list


================================================================================
LESSON 5 - EXTI EXTERNAL INTERRUPTS
================================================================================

CONCEPT
  Instead of polling a pin in a loop, EXTI hardware watches the pin.
  When pin changes state (edge detected), hardware fires an interrupt automatically.
  CPU is free to do other work. Button press interrupts it momentarily.
  Three hardware blocks must all be configured together.

THREE HARDWARE BLOCKS
  SYSCFG  Connects a specific GPIO port to an EXTI line.
          Without this, EXTI does not know which port to watch.
          PC13 = port C = value 0b0010 written into EXTICR4 bits [7:4].

  EXTI    Configures edge type and enables the interrupt line.
          EXTI_FTSR bit 13 = falling edge trigger (button press goes LOW)
          EXTI_IMR  bit 13 = unmask line 13 (allow interrupt to pass to NVIC)
          EXTI_PR   bit 13 = pending flag, must be cleared inside handler

  NVIC    ARM core interrupt controller. Must enable the specific IRQ number.
          EXTI15_10 = IRQ number 40 (from RM0390 Table 38)
          IRQ 40 is in NVIC_ISER1 (covers IRQs 32-63), bit position 40-32=8
          NVIC->ISER[1] |= (1U << 8)

SHARED HANDLERS
  Lines 10-15 share one handler: EXTI15_10_IRQHandler
  Inside handler, check EXTI_PR to know which specific line fired.
  Must clear EXTI_PR before returning or CPU is trapped in infinite interrupt loop.

HANDLER PATTERN
  void EXTI15_10_IRQHandler(void) {
      if (EXTI->PR & (1U << 13)) {
          // do your work here
          EXTI->PR |= (1U << 13);  // write 1 to clear (rc_w1 type bit)
      }
  }

KEY CONCEPTS LEARNED
  SYSCFG clock must be enabled: RCC_APB2ENR bit 14 = SYSCFGEN
  Vector table addresses: position x address = (16 + IRQ_number) x 4 bytes
  Pending register: write 1 to clear (opposite of most registers)
  NVIC_ISER: each bit enables one IRQ, writing 1 enables, writing 0 has no effect


================================================================================
LESSON 6 - ADC (ANALOG TO DIGITAL CONVERTER)
================================================================================

CONCEPT
  ADC converts analog voltage (0V to 3.3V) into a 12-bit number (0 to 4095).
  The STM32F446RE has three ADC units (ADC1, ADC2, ADC3).
  There is NOT one ADC core per pin.
  All pins share a single SAR (Successive Approximation Register) core
  via an internal analog multiplexer (AMUX).

PHYSICAL PINS SUPPORTING ADC (STM32F446RE LQFP64)
  16 external channels:
  CH0=PA0  CH1=PA1  CH2=PA2  CH3=PA3  CH4=PA4  CH5=PA5
  CH6=PA6  CH7=PA7  CH8=PB0  CH9=PB1
  CH10=PC0 CH11=PC1 CH12=PC2 CH13=PC3 CH14=PC4 CH15=PC5
  Plus internal: CH17=VREFINT, CH18=temperature sensor/VBAT

AMUX OPERATION
  AMUX switches to channel 0 -> SAR core converts -> result in ADC_DR
  AMUX switches to channel 1 -> SAR core converts -> result in ADC_DR (ch0 gone)
  AMUX switches to channel 2 -> SAR core converts -> result in ADC_DR (ch1 gone)
  ADC_DR holds exactly one value. Previous value is overwritten by next conversion.
  This is why DMA is essential for multi-channel reading.

THREE READING METHODS

  Polling (not recommended for multi-channel)
    CPU starts conversion with SWSTART
    CPU spins in while loop watching EOC flag in ADC_SR
    CPU reads ADC_DR when EOC=1
    CPU fully occupied, cannot do other work

  Interrupt (occasional slow measurements)
    ADC fires interrupt when conversion complete
    CPU jumps to handler, reads ADC_DR, returns
    Better than polling but interrupt overhead for high-speed scanning

  DMA + Circular (correct method for continuous multi-channel)
    ADC generates DMA request after each conversion
    DMA reads ADC_DR and writes to SRAM array automatically
    Array always contains latest value for each channel
    CPU never involved in data movement

KEY REGISTERS
  ADC_CR1   SCAN bit(8)=scan mode, RES bits(25:24)=resolution
  ADC_CR2   ADON(0)=power on, CONT(1)=continuous, DMA(8)=DMA enable,
            DDS(9)=DMA requests continue, SWSTART(30)=start conversion
  ADC_SMPR1 Sample time channels 10-18 (3 bits per channel)
  ADC_SMPR2 Sample time channels 0-9  (3 bits per channel)
            Value 4 = 84 cycles, safe default for most sensors
  ADC_SQR1  L[23:20]=number of conversions minus 1
  ADC_SQR3  SQ1[4:0] SQ2[9:5] SQ3[14:10] SQ4[19:15] SQ5[24:20] SQ6[29:25]
  ADC_SQR2  SQ7 through SQ12
  ADC_SR    EOC(1)=conversion complete, OVR(5)=overrun occurred
  ADC_DR    Read-only. Contains 12-bit conversion result.

GPIO FOR ADC
  Pins used for ADC must be in analog mode: GPIOx->MODER |= (3U << (pin*2))
  No PUPDR, OTYPER, or AFRL configuration needed for analog pins.


================================================================================
LESSON 7 - DMA (DIRECT MEMORY ACCESS)
================================================================================

CONCEPT
  DMA is a hardware controller that moves data between memory locations
  and peripherals without CPU involvement.
  ADC generates a DMA request after each conversion.
  DMA wakes up, reads ADC_DR, writes to your SRAM array, goes back to sleep.
  CPU is completely free during this entire process.

BUFFER BEHAVIOR
  The DMA buffer does NOT grow over time.
  It does NOT keep history.
  It OVERWRITES old data with new data.
  adc_buf[n] always contains the LATEST reading of the nth channel in sequence.
  Reading adc_buf[0] gives you the most recent value of the first channel.
  If you need history, copy values to a separate array in your application code.

DMA STREAM SELECTION FOR ADC1
  ADC1 must use DMA2 (not DMA1) - from RM0390 Table 29
  DMA2 Stream 0, Channel 0 is the standard assignment for ADC1

KEY DMA REGISTERS (DMA2 Stream 0)
  DMA_SxCR    Configuration register
              CHSEL[27:25] channel selection (0 for ADC1)
              PL[17:16]    priority (10=high)
              MSIZE[14:13] memory data size (01=16-bit half-word)
              PSIZE[12:11] peripheral data size (01=16-bit half-word)
              MINC(10)     memory increment (1=pointer advances each transfer)
              PINC(9)      peripheral increment (0=ADC_DR address stays fixed)
              CIRC(8)      circular mode (1=wrap back to start after NDTR transfers)
              DIR[7:6]     direction (00=peripheral to memory)
              EN(0)        enable (set last, after all other configuration)

  DMA_SxNDTR  Number of data transfers (set to number of ADC channels)
              In circular mode, auto-reloads when it reaches zero

  DMA_SxPAR   Peripheral address = address of ADC1->DR
              Fixed source that DMA always reads from

  DMA_SxM0AR  Memory address = address of your adc_buf[] array
              DMA writes here and increments by MSIZE after each transfer

CIRCULAR MODE OPERATION
  DMA transfers NDTR items (one full scan sequence)
  When NDTR reaches zero, DMA reloads NDTR to original value
  Memory pointer resets back to start of adc_buf
  Process repeats forever
  ADC CONT mode restarts scan from channel 1 simultaneously

HOW CPU READS DATA
  volatile uint16_t adc_buf[8];   // volatile mandatory - DMA writes outside CPU flow
  uint16_t sensor = adc_buf[0];   // just read the array, no waiting, no polling
  // DMA has kept this fresh continuously in the background


================================================================================
LESSON 8 - STM32 MEMORY AND FLASHING
================================================================================

MEMORY LAYOUT
  Flash   512KB   0x08000000   Program storage. Permanent. Survives power off.
                               Your compiled .elf/.bin file is written here.
                               Vector table lives at the very start.
  SRAM    128KB   0x20000000   Runtime data. Variables, stack, heap.
                               Lost on power off.
  System  ~30KB   0x1FFF0000   ST factory bootloader. Read only. Never erased.
  Memory                       Activated by BOOT0 pin.

TWO WAYS TO FLASH THE STM32

  Method 1 - SWD via ST-LINK (your normal workflow)
    Uses PA13 (SWDIO) and PA14 (SWCLK) - dedicated ARM debug pins
    CubeIDE -> USB -> ST-LINK chip -> SWD -> Flash controller -> Flash
    Supports full debugging: breakpoints, register inspection, stepping
    BOOT0 state does not matter for this method

  Method 2 - UART Bootloader (factory bootloader in System Memory)
    Set BOOT0 pin HIGH before reset
    Chip boots into ST factory bootloader instead of your program
    Bootloader listens on USART1 (PA9/PA10), USART2 (PA2/PA3), USB, CAN2
    Host tool (STM32CubeProgrammer) sends firmware over one of these interfaces
    Bootloader writes firmware to Flash
    After flashing, set BOOT0 LOW, reset, your program runs

BOOT0 PIN BEHAVIOR
  BOOT0 = LOW at reset   Your program in Flash runs normally
  BOOT0 = HIGH at reset  Factory bootloader runs, your program paused

IS USART SAFE TO USE AS NORMAL COMMUNICATION?
  Completely safe.
  Bootloader only activates when BOOT0 is HIGH at reset.
  During normal operation (BOOT0 LOW), bootloader never runs.
  PA9/PA10 or PA2/PA3 can be used as normal USART freely.
  The two modes are mutually exclusive.

NUCLEO BOARD ST-LINK
  The small section of the Nucleo board is a separate STM32F103 chip.
  It acts as a USB-to-SWD bridge.
  Plugging USB into Nucleo = connecting an ST-LINK programmer.
  No external programmer needed for development.
  Can be separated from the main board (cut the PCB) for deployment.


================================================================================
ARCHITECTURAL CONCEPTS LEARNED THROUGHOUT
================================================================================

MEMORY-MAPPED I/O
  CPU has one unified address space from 0x00000000 to 0xFFFFFFFF.
  Hardware registers occupy addresses in the peripheral region 0x40000000+.
  Writing to these addresses physically changes hardware state.
  Reading from them physically samples hardware state.
  No special CPU instructions needed - ordinary load/store instructions work.

CLOCK GATING
  Every peripheral is frozen by default to save power.
  RCC (Reset and Clock Control) is the switchboard.
  You must enable each peripheral's clock before using it.
  Writing to a frozen peripheral's registers has no effect and no error.
  This is the most common cause of "my peripheral does not work" problems.

VOLATILE KEYWORD
  Required for all hardware register accesses.
  Required for any variable written by an interrupt and read by main code.
  Required for any variable written by DMA and read by CPU.
  Without volatile, compiler may cache values in CPU registers and miss updates.

BUS ARCHITECTURE
  AHB1: GPIO ports, DMA controllers (high speed, full CPU clock)
  APB1: USART2-5, TIM2-7, SPI2-3, I2C (slower, max 45MHz)
  APB2: USART1, ADC, TIM1, SPI1 (medium speed, max 90MHz)
  Bus determines which RCC enable register to use.

READ-MODIFY-WRITE PATTERN
  Always used when changing specific bits without disturbing others.
  Clear first:  REG &= ~(mask << position)   forces target bits to 0
  Then set:     REG |=  (value << position)   writes desired value
  Never use =   (assignment) for multi-bit fields in shared registers.

VECTOR TABLE
  Array of 32-bit function pointers at start of Flash (0x00000000).
  First 16 entries: ARM core exceptions (Reset, NMI, HardFault, SysTick...)
  Remaining entries: chip-specific peripheral interrupts
  Entry address = (16 + IRQ_number) x 4
  Startup file builds this table. Your handler name replaces weak default.

WEAK SYMBOLS
  Startup file declares every handler as weak (default = infinite loop).
  When you define a function with the exact same name, linker uses yours.
  This is how SysTick_Handler and EXTI15_10_IRQHandler work.
  No manual vector table editing required.

ALTERNATE FUNCTION SYSTEM
  Every GPIO pin connects internally to multiple peripherals via AMUX.
  MODER = 0b10: disconnects GPIO, connects pin to AF multiplexer.
  AFRL/AFRH: selects which peripheral (AF0-AF15) gets the pin.
  Both registers required. Wrong AF number = peripheral gets wrong pin.
  AF mapping found in datasheet alternate function table.


================================================================================
DOCUMENT NAVIGATION CHEAT SHEET
================================================================================

QUESTION                               DOCUMENT AND SECTION
Which pin supports which function?     Datasheet Table 11
Which AF number for which peripheral?  Datasheet Table 11, AF columns
Which bus does peripheral X use?       RM0390 Section 2.2 Table 1
Base address of peripheral X?          RM0390 Section 2.2 Table 1
Clock enable bit for peripheral X?     RM0390 Section 6.3 (RCC chapter)
Register offsets for peripheral X?     RM0390 peripheral chapter register map
What does each bit in register Y do?   RM0390 register Y description page
SysTick registers?                     DUI0553 Section 4.4
NVIC registers?                        DUI0553 Section 4.2
Vector table (IRQ numbers)?            RM0390 Table 38
DMA stream to use for ADC1?            RM0390 Table 29


================================================================================
ROADMAP - WHERE YOU ARE AND WHAT IS NEXT
================================================================================

COMPLETED
  GPIO Output          LED blink, raw register access, bitwise operations
  GPIO Input           Button reading, IDR, PUPDR, polling
  SysTick              Millisecond timer, first interrupt handler, delay_ms
  USART                Serial print, pin multiplexing, AF registers, printf
  EXTI                 External interrupts, SYSCFG, NVIC, edge detection
  ADC concepts         AMUX architecture, three reading methods, DMA theory
  DMA concepts         Circular buffer, data flow, register overview
  Memory and Flashing  Flash/SRAM/System memory, SWD, UART bootloader

NEXT STEPS
  ADC + DMA practical  Write and test actual multi-channel DMA ADC code
  PWM                  Timer-based pulse width modulation for motors and servos
  CMSIS migration      Rewrite raw pointer code using CMSIS struct syntax
  First real project   LFR or sensor project combining ADC + USART + GPIO
  I2C (optional)       For sensors requiring I2C protocol
  RTOS (future)        FreeRTOS on top of everything you now understand


================================================================================
THE MOST IMPORTANT THING YOU LEARNED
================================================================================

The methodology never changes regardless of which chip or peripheral you use:

  Step 1  Open datasheet - find the physical pin and AF number
  Step 2  Open RM Section 2.2 - find the bus and base address
  Step 3  Open RCC chapter - find and enable the clock
  Step 4  Open peripheral chapter register map - find offsets
  Step 5  Open individual register pages - find bit meanings
  Step 6  Open functional description - understand the behavior
  Step 7  Write the code - base address plus offset, bitwise operations

Every embedded system you will ever work on follows this same path.
The documents contain every answer.
The skill is knowing which document answers which type of question.

================================================================================
END OF SUMMARY
================================================================================
