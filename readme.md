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


# Academic Technical Report: Multi-Channel ADC Architecture with DMA Integration

**Module:** Embedded Systems Hardware Design

**Target Architecture:** ARM Cortex-M4 (STM32F4xx Peripheral Architecture)

---

## 1. Introduction to Mixed-Signal Data Acquisition

In modern embedded systems, interfacing with the physical world requires processing continuous analog signals (such as voltage, temperature, or pressure) and converting them into discrete digital representations. This task is executed by the **Analog-to-Digital Converter (ADC)**.

When an academic or production application demands the monitoring of multiple analog nodes simultaneously, standard sequential software techniques introduce significant latency, determinism degradation, and CPU execution overhead. To eliminate these bottlenecks, advanced microcontroller architectures integrate a high-performance **Direct Memory Access (DMA)** controller to manage data transport entirely within the hardware domain.

---

## 2. Hardware Microarchitecture & The Multiplexing Constraint

### 2.1 Core Component Architecture

A common architectural misconception is that every external physical pin capable of analog sampling is backed by an independent, dedicated ADC hardware core. In physical silicon implementations, this approach is highly inefficient due to the immense silicon area required for high-resolution conversion stages.

Instead, the microcontroller utilizes a centralized **Successive Approximation Register (SAR) ADC engine**. This single core is shared across multiple external physical pins using an internal **Analog Multiplexer (MUX)**.

### 2.2 The Single Data Register Bottleneck

Because a singular SAR engine processes all incoming channels sequentially, the peripheral design contains only **one shared 32-bit Data Register (`ADC_DR`)**.

When the MUX routes an input pin to the SAR engine, the conversion occurs over a set number of clock cycles. The moment the digital value is finalized:

1. The result is written directly into `ADC_DR`.
2. Any data previously residing within `ADC_DR` from a prior channel conversion is instantly overwritten.

Without a mechanism to offload this data immediately upon completion, the system encounters an **Overrun Error (`OVR` flag active)**, resulting in catastrophic loss of data sequence integrity.

---

## 3. Comparative Analysis of Data Management Methodologies

To appreciate the efficiency of a DMA-driven architecture, we must analyze the data flow maps and CPU utilization of the three primary operational modes of the peripheral.

### 3.1 Single Conversion Mode (CPU Polling)

In this mode, the CPU is entirely tied to the state machine of the peripheral.

```
[CPU Writes Channel to MUX] ──> [CPU Triggers Start] ──> [CPU Loops/Polls 'End of Conversion' Flag] ──> [CPU Reads ADC_DR]

```

* **Efficiency:** Extremely low. The CPU enters a blocking execution loop, wasting hundreds of instruction cycles while waiting for the analog sampling capacitor to charge and converge.

### 3.2 Scan Mode via Interrupt Service Routines (CPU Interrupts)

The ADC hardware automatically cycles the MUX through a pre-defined sequence of channels (**Scan Mode**). As each individual channel completes its conversion, a hardware interrupt is fired.

```
[ADC Converts Ch0] ──> [Hardware Interrupt] ──> [CPU Suspends Main Task] ──> [ISR Reads ADC_DR to RAM] ──> [Context Restore]

```

* **Efficiency:** Moderate for low sample rates, highly inefficient for high-frequency sampling. The overhead of context switching (saving and restoring registers to the stack) for every single pin conversion introduces massive determinism jitter into the main application loop.

### 3.3 Scan Mode integrated with DMA (Hardware-Automated Data Flow)

This represents the production-grade standard. The CPU configures the initial hardware routing and memory boundaries once. After initialization, the CPU drops out of the data movement equation entirely.

```
┌────────────────────────────────────────────────────────┐
│                      ANALOG PINS                       │
│   PA0     PA1     PA2     PA3     PA4     PA5     PA6  │
└───┬───────┬───────┬───────┬───────┬───────┬───────┬────┘
    │       │       │       │       │       │       │
    ▼       ▼       ▼       ▼       ▼       ▼       ▼
┌────────────────────────────────────────────────────────┐
│                 Analog Multiplexer (MUX)               │
└──────────────────────────┬─────────────────────────────┘
                           │ Configured Sequence Routing
                           ▼
┌────────────────────────────────────────────────────────┐
│                   SAR ADC Engine Core                  │
└──────────────────────────┬─────────────────────────────┘
                           │ Writes Conversion Result
                           ▼
┌────────────────────────────────────────────────────────┐
│                ADC Data Register (ADC_DR)              │
└──────────────────────────┬─────────────────────────────┘
                           │ Hardware Generates DMA Request Pulse
                           ▼
┌────────────────────────────────────────────────────────┐
│                 DMA Controller Stream                  │
└──────────────────────────┬─────────────────────────────┘
                           │ Direct Bus Master Write (Zero CPU)
                           ▼
┌────────────────────────────────────────────────────────┐
│             SRAM Destination Array (RAM)               │
│  adc_buffer[0] <── (Channel A result)                  │
│  adc_buffer[1] <── (Channel B result)                  │
│  adc_buffer[2] <── (Channel C result)                  │
└────────────────────────────────────────────────────────┘

```

---

## 4. Deep Dive: Memory Synchronicity and DMA Mechanics

To implement a stable data acquisition system, we must address the exact hardware synchronization mechanics governing the DMA controller.

### 4.1 Boundary Management & Circular Buffering

The DMA controller does not inherently understand high-level C-language data structures, array boundaries, or types. It executes purely based on memory addresses and register-level constraints supplied during configuration:

1. **Peripheral Base Address:** Permanently locked to the physical address of the ADC Data Register (`&ADC_DR`).
2. **Memory Base Address:** The base pointer of the target destination array allocated in Static RAM (`&adc_buffer[0]`).
3. **Hardware Transfer Counter:** The absolute count of elements to transfer before resetting (e.g., `8`).

When configured in **Circular Mode**, the DMA controller maintains an internal countdown register and an internal destination address pointer:

$$\text{Internal Destination Pointer} = \text{Memory Base Address} + (\text{Current Index} \times \text{Data Size})$$

* **Step-by-Step Execution Loop:**
* **Conversion Completion:** The ADC completes a channel conversion, updates `ADC_DR`, and asserts a hardware DMA request line.
* **Data Transport:** The DMA engine takes control of the system bus master, reads `ADC_DR`, and writes that data to the memory address indicated by its internal pointer.
* **Pointer Increment:** The DMA increments its memory pointer to prepare for the *next* array slot, and decrements its internal Transfer Counter by 1.
* **The Wrap-Around:** When the internal Transfer Counter decrements to `0`, the DMA hardware automatically reloads the initial Transfer Counter value (e.g., `8`) and resets its internal memory pointer back to the exact Memory Base Address.



Consequently, the memory footprint does **not** expand over time. It forms a persistent loop in SRAM where the oldest data point for a given channel is cleanly overwritten by its newest counterpart.

### 4.2 Sequence Mapping Theory

There is no hardware-level mechanism tying an array index natively to a physical pin identifier. The index mapping is entirely a product of the channel compilation order specified inside the **ADC Sequence Registers (SQR)**.

The SQR peripheral block functions as an execution playlist:

$$\text{ADC\_SQR Slot Number} \Longleftrightarrow \text{DMA Destination Array Index}$$

If a designer programs the sequence registers with an arbitrary channel layout, the DMA will strictly write them into memory in chronological order of conversion:

| Conversion Step | SQR Sequence Target | Assigned Channel | Physical Pin | Target Destination Memory Index |
| --- | --- | --- | --- | --- |
| **Step 1** | Slot 1 (`SQ1`) | Channel 0 | `PA0` | `adc_buffer[0]` |
| **Step 2** | Slot 2 (`SQ2`) | Channel 4 | `PA4` | `adc_buffer[1]` |
| **Step 3** | Slot 3 (`SQ3`) | Channel 11 | `PC1` | `adc_buffer[2]` |

---

## 5. Architectural Map of Configuration Registers

To translate these conceptual models into firmware execution, the specific operational domains of the peripheral registers must be thoroughly mapped out.

### 5.1 Peripheral Control Blocks

* **`ADC_CR1` (Control Register 1):** Conveys global configuration parameters. Crucially, the `SCAN` bit must be toggled high to ensure the internal MUX steps sequentially through the entire programmed playlist instead of halting after evaluating the first slot.
* **`ADC_CR2` (Control Register 2):** Controls execution states and hardware triggering. The `DMA` bit must be asserted to route conversion-complete flags directly to the DMA engine controller. The `DDS` (DMA Disable Selection) bit must be forced high to ensure continuous, infinite DMA requests are issued across successive wrap-around scan sequences. It also contains the `ADON` (ADC Enable) bit and `SWSTART` (Software Start Trigger).
* **`ADC_SMPRx` (Sample Time Registers):** Manages the physical sample-and-hold analog window. Each channel can be assigned a specific number of ADC clock cycles dedicated to charging the internal sampling capacitor, guaranteeing noise filtering and accuracy control over different sensor impedances.

### 5.2 Playlist Sequence Management

* **`ADC_SQR1`:** Manages sequence slots 13 to 16, and holds the 4-bit `L` field. The `L` field must be written with the total number of conversions to be executed in a single loop minus 1:

$$\text{Value Written to L} = \text{Total Channels} - 1$$


* **`ADC_SQR2`:** Holds the channel routing mappings for sequence slots 7 through 12.
* **`ADC_SQR3`:** Holds the channel routing mappings for the initial sequence slots 1 through 6. This is where the primary execution sequence begins.

### 5.3 Shared Peripherals & Timing Context

* **`ADC_CCR` (Common Control Register):** A shared global configuration register across all independent ADC engines. It houses the clock prescaler settings (`ADCPRE`). It ensures that the clock derived from the high-speed peripheral bus does not exceed the maximum operational input frequency defined by the silicon specification (typically 36 MHz for core stability).

---

## 6. Software Engineering Best Practices for Verification

To prevent buffer overflow errors and remove brittle, unmaintainable literal integers ("magic numbers") from application layers, engineers employ strongly typed abstractions.

By leveraging C-language enumerations (`enum`), the hardware sequence configuration matches the software index lookup syntax cleanly:

```c
/**
 * @brief Explicit enumeration mapping hardware sequence slots to software array positions.
 */
typedef enum {
    ADC_INDEX_PRESSURE = 0,  // Configured in SQR3 -> SQ1 Slot
    ADC_INDEX_BATTERY  = 1,  // Configured in SQR3 -> SQ2 Slot
    ADC_INDEX_THERMAL  = 2,  // Configured in SQR3 -> SQ3 Slot
    ADC_SEQ_TOTAL            // Automatically evaluates to 3
} ADC_Sequence_Layout;

/* Define fixed-size hardware buffer directly correlated with the sequence count */
uint16_t g_adc_raw_buffer[ADC_SEQ_TOTAL];

/**
 * @brief High-level application thread demonstrating zero overhead reading.
 */
void Execute_Control_Loop(void) {
    // The CPU reads directly from SRAM with zero polling or register bus latencies
    uint16_t current_pressure = g_adc_raw_buffer[ADC_INDEX_PRESSURE];
    uint16_t system_voltage   = g_adc_raw_buffer[ADC_INDEX_BATTERY];
    
    if (system_voltage < 3300) {
        // Safe execution routines for undervoltage state
    }
}

```
