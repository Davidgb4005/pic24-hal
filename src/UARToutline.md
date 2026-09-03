# PIC24F16KA101 UART HAL — Implementation Outline

## Target

Implement an interrupt-driven, non-blocking UART HAL for:

* MCU: PIC24F16KA101
* Language: C
* Compiler: XC16
* Device definitions: XC16 PIC24F16KA101 headers
* Existing HAL project

Implement:

```text
include/
    uart.h

src/
    uart.c

examples/
    uart_example.c
```

Do not perform:

* toolchain configuration
* compiler installation
* programmer configuration
* hardware testing
* unit testing
* CMake restructuring
* MPLAB configuration

I will handle compiling, programming, and hardware verification.

Keep the implementation PIC24F16KA101-specific and simple.

Do not add generic cross-MCU abstraction layers.

---

# 1. UART Instances

The PIC24F16KA101 provides two UART peripherals.

Expose:

```c
typedef enum {
    UART_1,
    UART_2,

    UART_COUNT
} uart_t;
```

The public API should not expose registers such as:

```text
U1MODE
U1STA
U1BRG
U1TXREG
U1RXREG
```

---

# 2. UART Configuration

Use a minimal configuration structure:

```c
typedef struct {
    uint32_t baud_rate;
} uart_config_t;
```

The initial UART implementation uses:

```text
8 data bits
no parity
1 stop bit
```

Do not expose configurable:

* parity
* stop bits
* character size
* hardware flow control
* inversion
* LIN
* IrDA

These can be added later if required.

---

# 3. Initialization

Provide:

```c
hal_status_t uartInit(
    uart_t uart,
    pin_t tx_pin,
    pin_t rx_pin,
    const uart_config_t *config
);
```

Example:

```c
uart_config_t uart_config = {
    .baud_rate = 115200
};

uartInit(
    UART_1,
    PIN_6,
    PIN_7,
    &uart_config
);
```

The actual valid physical pins must be determined from the PIC24F16KA101 datasheet and XC16 device definitions.

Do not guess PPS mappings.

---

# 4. Initialization Sequence

`uartInit()` should:

1. Disable the UART while configuring it.
2. Claim the TX physical pin with:

```c
PIN_OWNER_UART
```

3. Claim the RX physical pin with:

```c
PIN_OWNER_UART
```

4. Configure the pins as digital pins.
5. Configure the TX pin direction appropriately.
6. Configure the RX pin direction appropriately.
7. Configure Peripheral Pin Select if required.
8. Calculate the baud-rate generator value using the system instruction-cycle frequency.
9. Configure UART for 8-N-1.
10. Clear UART error state.
11. Clear RX and TX interrupt flags.
12. Configure RX interrupt.
13. Configure TX interrupt.
14. Initialize the software TX and RX ring buffers.
15. Enable the UART peripheral.
16. Enable the UART transmitter.
17. Enable RX interrupts.
18. Leave TX interrupts disabled until data is queued.

Return an appropriate `hal_status_t` if initialization fails.

---

# 5. Clock Source

UART baud-rate calculations must use the system clock HAL.

Conceptually:

```c
uint32_t fcy = clockGetFCY();
```

Do not hard-code oscillator or instruction-cycle frequencies in `uart.c`.

The implementation should calculate the required UART BRG value from:

```text
FCY
baud rate
UART BRG mode
```

Use the simplest suitable BRG mode.

The application only specifies:

```c
.baud_rate = 115200
```

Do not expose BRG details publicly.

---

# 6. Non-Blocking Architecture

Both transmission and reception must be interrupt driven.

Use:

```text
Application
    |
    | uartWrite()
    v
TX ring buffer
    |
    | TX interrupt
    v
UART hardware


UART hardware
    |
    | RX interrupt
    v
RX ring buffer
    |
    | uartRead()
    v
Application
```

Application code must not normally wait for UART hardware.

---

# 7. Fixed Ring Buffers

Use fixed-size statically allocated ring buffers.

Do not use dynamic allocation.

Define configurable compile-time sizes in `uart.h` or internally:

```c
#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64
```

A different reasonable small fixed size is acceptable.

Each UART instance must have its own TX and RX buffers.

Conceptually:

```c
typedef struct {
    uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;

    uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

    pin_t tx_pin;
    pin_t rx_pin;

    bool initialized;
} uart_runtime_t;
```

Create:

```c
static uart_runtime_t uart_state[UART_COUNT];
```

Keep the ring-buffer implementation simple.

---

# 8. Non-Blocking TX

Provide:

```c
bool uartWriteByte(
    uart_t uart,
    uint8_t data
);
```

Behavior:

* place the byte in the TX ring buffer
* enable the UART TX interrupt
* return immediately

Return:

```c
true
```

if the byte was successfully queued.

Return:

```c
false
```

if the TX buffer is full.

Do not block waiting for buffer space.

---

# 9. Multi-Byte TX

Provide:

```c
uint16_t uartWrite(
    uart_t uart,
    const uint8_t *data,
    uint16_t length
);
```

Attempt to queue as many bytes as possible.

Return the number of bytes actually queued.

Example:

```c
uint8_t data[] = {1, 2, 3, 4};

uint16_t written =
    uartWrite(UART_1, data, sizeof(data));
```

If only three bytes fit:

```text
return value = 3
```

Do not wait for the fourth byte to become available.

---

# 10. String Output

Provide:

```c
uint16_t uartPrint(
    uart_t uart,
    const char *string
);
```

Queue characters until:

* `'\0'` is reached, or
* the TX buffer becomes full

Return the number of characters queued.

Example:

```c
uartPrint(UART_1, "Hello\r\n");
```

This function must remain non-blocking.

---

# 11. TX Interrupt

Implement UART TX interrupt handlers for UART1 and UART2.

The TX ISR should:

1. Check whether bytes remain in the TX ring buffer.
2. Move queued bytes from the software buffer into the UART TX hardware while hardware space is available.
3. Advance the TX ring-buffer tail.
4. When the software TX buffer becomes empty, disable the TX interrupt.

The TX interrupt should remain disabled when there is no data to transmit.

Calling:

```c
uartWriteByte()
uartWrite()
uartPrint()
```

should enable the TX interrupt after placing data in the buffer.

This provides the transition:

```text
TX idle
   |
uartWrite()
   |
   v
TX interrupt enabled
   |
   v
buffer transmitted
   |
   v
buffer empty
   |
   v
TX interrupt disabled
```

Do not busy-wait inside the ISR.

---

# 12. TX Buffer State

Provide:

```c
uint16_t uartTxAvailable(uart_t uart);
```

Return the amount of free space remaining in the TX software buffer.

Also provide:

```c
bool uartTxBusy(uart_t uart);
```

Return `true` if either:

* bytes remain in the software TX queue, or
* UART hardware is still transmitting queued data

This allows application code to determine whether transmission has completely finished without blocking.

---

# 13. Interrupt-Driven RX

The UART RX interrupt must continuously move received bytes from the UART hardware into the RX software ring buffer.

Application code does not need to manually service the UART peripheral.

The RX ISR should:

1. detect received data
2. read received bytes from the hardware RX register/FIFO
3. place them in the RX ring buffer
4. advance the RX head
5. clear the appropriate interrupt condition

Do not call application callbacks from the ISR.

---

# 14. RX Availability

Provide:

```c
uint16_t uartAvailable(uart_t uart);
```

Return the number of bytes currently available in the RX software buffer.

Example:

```c
if (uartAvailable(UART_1) > 0) {
    ...
}
```

This function must not block.

---

# 15. Non-Blocking RX

Provide:

```c
bool uartReadByte(
    uart_t uart,
    uint8_t *data
);
```

Behavior:

If data exists:

```c
*data = next received byte;
return true;
```

If no data exists:

```c
return false;
```

Do not block waiting for incoming UART data.

Example:

```c
uint8_t byte;

if (uartReadByte(UART_1, &byte)) {
    // byte received
}
```

---

# 16. Multi-Byte RX

Provide:

```c
uint16_t uartRead(
    uart_t uart,
    uint8_t *data,
    uint16_t max_length
);
```

Read up to `max_length` bytes currently available.

Return the number of bytes actually read.

Do not wait for additional data.

Example:

```c
uint8_t buffer[32];

uint16_t received =
    uartRead(UART_1, buffer, sizeof(buffer));
```

If five bytes are currently available:

```text
received = 5
```

and the function immediately returns.

---

# 17. RX Buffer Overflow

The RX software buffer can become full if the application does not read incoming data quickly enough.

Use a simple policy:

```text
keep existing buffered data
discard newly received byte
set overflow flag
```

Do not overwrite unread data.

Maintain an overflow state for each UART.

Provide:

```c
bool uartRxOverflow(uart_t uart);
```

Return whether a software RX buffer overflow has occurred.

Provide:

```c
void uartClearRxOverflow(uart_t uart);
```

to clear the software overflow flag.

---

# 18. Hardware Receive Errors

The RX ISR must correctly handle PIC24 UART hardware receive errors, particularly hardware overrun.

If the UART enters an overrun state, clear it using the procedure required by the PIC24F16KA101 hardware.

The UART must continue receiving afterward.

Do not create a complicated error framework.

Hardware framing/parity errors may simply cause the affected byte to be discarded in the first implementation.

---

# 19. Pin Ownership

Add/use:

```c
PIN_OWNER_UART
```

UART initialization must claim both pins:

```c
pinClaim(tx_pin, PIN_OWNER_UART);
pinClaim(rx_pin, PIN_OWNER_UART);
```

If either pin belongs to another peripheral, return:

```c
HAL_BUSY
```

If TX is successfully claimed but RX cannot be claimed, release TX before returning the error.

Do not leave partially claimed resources.

---

# 20. Peripheral Pin Select

UART pin routing should be handled internally.

The public API uses physical package pins:

```c
uartInit(
    UART_1,
    PIN_6,
    PIN_7,
    &config
);
```

The user should not specify:

```text
RAx
RBx
RPx
PPS register numbers
UART PPS function codes
```

If the PIC24F16KA101 UART uses Peripheral Pin Select for the selected signals, map the physical pin internally.

Extend the internal physical pin mapping if necessary with information such as:

```c
typedef struct {
    ...
    uint8_t rp_number;
} pin_hw_t;
```

Use the actual device datasheet and XC16 register definitions.

Do not guess RP numbers or PPS function codes.

---

# 21. PPS Validation

UART initialization should ensure that the selected physical pin can actually perform the requested UART function.

Unlike basic GPIO, UART pin mapping depends on peripheral-routing capability.

If the requested TX or RX pin cannot be mapped to that UART peripheral, return an appropriate HAL error.

Do not write an invalid PPS configuration.

---

# 22. Deinitialization

Provide:

```c
void uartDeinit(uart_t uart);
```

This should:

1. disable UART interrupts
2. disable the UART peripheral
3. disable the UART transmitter
4. clear UART interrupt flags
5. clear TX/RX ring-buffer state
6. disconnect PPS mappings if appropriate
7. release the TX pin
8. release the RX pin
9. mark the UART as uninitialized

Do not affect the other UART peripheral.

---

# 23. Interrupt Safety

Ring-buffer indices are shared between:

```text
application code
UART interrupt handlers
```

Use `volatile` where necessary.

Keep operations on shared indices simple and atomic for the PIC24 architecture.

Avoid globally disabling interrupts unless actually necessary.

Do not introduce mutexes, locks, RTOS primitives, or dynamic synchronization.

---

# 24. ISR Scope

Implement the actual UART1 and UART2 interrupt service routines inside `uart.c`.

Conceptually:

```c
_U1RXInterrupt()
_U1TXInterrupt()

_U2RXInterrupt()
_U2TXInterrupt()
```

Use the correct XC16 interrupt declarations and actual PIC24F16KA101 interrupt-vector names from the device headers.

Do not guess ISR names or attributes.

ISR responsibilities must remain small:

```text
RX ISR:
hardware -> RX buffer

TX ISR:
TX buffer -> hardware
```

Do not perform:

* string formatting
* protocol parsing
* callbacks
* application logic
* delays

inside an ISR.

---

# 25. Interrupt Priority

Use a fixed UART interrupt priority initially.

Define it in one location, for example:

```c
#define UART_INTERRUPT_PRIORITY 4
```

Use the actual legal PIC24 interrupt priority range.

Do not expose interrupt priority through the public UART API in the first implementation.

---

# 26. Buffer Reset

Provide:

```c
void uartFlushRx(uart_t uart);
```

This discards all unread bytes currently stored in the RX software buffer.

Do not confuse this with TX completion.

Do not create a blocking `uartFlush()` function.

TX completion can instead be checked through:

```c
uartTxBusy()
```

---

# 27. Public API

The initial `uart.h` interface should approximately be:

```c
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "pin.h"

#define UART_TX_BUFFER_SIZE 64
#define UART_RX_BUFFER_SIZE 64

typedef enum {
    UART_1,
    UART_2,

    UART_COUNT
} uart_t;

typedef struct {
    uint32_t baud_rate;
} uart_config_t;


hal_status_t uartInit(
    uart_t uart,
    pin_t tx_pin,
    pin_t rx_pin,
    const uart_config_t *config
);

void uartDeinit(uart_t uart);


/* TX */

bool uartWriteByte(
    uart_t uart,
    uint8_t data
);

uint16_t uartWrite(
    uart_t uart,
    const uint8_t *data,
    uint16_t length
);

uint16_t uartPrint(
    uart_t uart,
    const char *string
);

uint16_t uartTxAvailable(uart_t uart);

bool uartTxBusy(uart_t uart);


/* RX */

uint16_t uartAvailable(uart_t uart);

bool uartReadByte(
    uart_t uart,
    uint8_t *data
);

uint16_t uartRead(
    uart_t uart,
    uint8_t *data,
    uint16_t max_length
);

void uartFlushRx(uart_t uart);

bool uartRxOverflow(uart_t uart);

void uartClearRxOverflow(uart_t uart);


#endif
```

---

# 28. Internal UART State

Use one runtime structure per UART:

```c
typedef struct {
    uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;

    uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

    pin_t tx_pin;
    pin_t rx_pin;

    volatile bool rx_overflow;
    bool initialized;
} uart_runtime_t;
```

Create:

```c
static uart_runtime_t uart_state[UART_COUNT];
```

Do not dynamically allocate buffers.

---

# 29. Internal UART Hardware Mapping

Use either:

* a small internal hardware mapping table, or
* straightforward `switch (uart)` functions

to access UART1/UART2 registers.

Prefer whichever is simpler with the XC16 register definitions.

Conceptually the mapping includes:

```text
MODE register
STATUS register
BRG register
TX register
RX register
interrupt flags
interrupt enables
interrupt priorities
```

Do not over-engineer the abstraction.

---

# 30. Example

Create:

```text
examples/uart_example.c
```

Example usage:

```c
#include "uart.h"

int main(void)
{
    uart_config_t config = {
        .baud_rate = 115200
    };

    uartInit(
        UART_1,
        PIN_6,
        PIN_7,
        &config
    );

    uartPrint(UART_1, "UART ready\r\n");

    while (1) {
        uint8_t byte;

        if (uartReadByte(UART_1, &byte)) {
            uartWriteByte(UART_1, byte);
        }
    }

    return 0;
}
```

The echo is entirely non-blocking:

```text
RX hardware
    ↓ interrupt
RX ring buffer
    ↓ uartReadByte()
application
    ↓ uartWriteByte()
TX ring buffer
    ↓ interrupt
TX hardware
```

Do not add delays, test frameworks, programmer commands, or UART debug wrappers.

---

# 31. Implementation Order

Implement in this order:

1. `uart_t`
2. UART runtime state
3. ring-buffer helpers
4. UART1/UART2 hardware access
5. baud-rate calculation
6. UART pin/PPS mapping
7. `uartInit()`
8. RX ISR
9. `uartAvailable()`
10. `uartReadByte()`
11. `uartRead()`
12. RX overflow handling
13. TX ISR
14. `uartWriteByte()`
15. `uartWrite()`
16. `uartPrint()`
17. `uartTxAvailable()`
18. `uartTxBusy()`
19. `uartFlushRx()`
20. `uartDeinit()`
21. `uart_example.c`

Keep the implementation interrupt-driven and non-blocking.

Do not add callbacks, DMA, blocking read/write functions, protocol parsing, `printf` integration, RTOS support, dynamic allocation, or generic serial-port abstractions.

