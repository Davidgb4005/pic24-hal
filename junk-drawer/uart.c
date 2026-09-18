#include "uart.h"

#include <xc.h>

#define UART_INTERRUPT_PRIORITY 4u
#define UART_BAUD_ERROR_PPM_MAX 50000ull

typedef struct {
    uint16_t brg;
    uint8_t brgh;
} uart_baud_config_t;

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

static uart_runtime_t uart_state[UART_COUNT];

static uint16_t ring_next(uint16_t index, uint16_t size)
{
    index++;
    if (index >= size) {
        index = 0u;
    }
    return index;
}

static uint16_t ring_count(volatile uint16_t *head, volatile uint16_t *tail, uint16_t size)
{
    uint16_t current_head = *head;
    uint16_t current_tail = *tail;

    if (current_head >= current_tail) {
        return current_head - current_tail;
    }

    return (uint16_t)(size - current_tail + current_head);
}

static bool uart_valid(uart_t uart)
{
    return uart < UART_COUNT;
}

static bool uart_pins_valid(uart_t uart, pin_t tx_pin, pin_t rx_pin)
{
    switch (uart) {
    case UART_1:
        return (tx_pin == PIN_11) && (rx_pin == PIN_6);

    case UART_2:
        return (tx_pin == PIN_4) && (rx_pin == PIN_5);

    default:
        return false;
    }
}

static void uart_configure_pins(uart_t uart)
{
    switch (uart) {
    case UART_1:
        TRISBbits.TRISB7 = 0;
        TRISBbits.TRISB2 = 1;
        break;

    case UART_2:
        AD1PCFGbits.PCFG2 = 1;
        AD1PCFGbits.PCFG3 = 1;
        TRISBbits.TRISB0 = 0;
        TRISBbits.TRISB1 = 1;
        break;

    default:
        break;
    }
}

static bool uart_brg_candidate(uint32_t baud_rate, uint8_t brgh, uart_baud_config_t *candidate, uint64_t *error_ppm)
{
    uint64_t divisor = (brgh != 0u) ? 4ull : 16ull;
    uint64_t value;
    uint64_t actual_baud;
    uint64_t error;

    value = (((uint64_t)HAL_FCY + ((divisor * baud_rate) / 2ull)) / (divisor * baud_rate));
    if ((value == 0u) || (value > 65536ull)) {
        return false;
    }

    actual_baud = (uint64_t)HAL_FCY / (divisor * value);
    if (actual_baud >= baud_rate) {
        error = actual_baud - baud_rate;
    } else {
        error = baud_rate - actual_baud;
    }

    candidate->brg = (uint16_t)(value - 1ull);
    candidate->brgh = brgh;
    *error_ppm = (error * 1000000ull) / baud_rate;

    return true;
}

static bool uart_brg(uint32_t baud_rate, uart_baud_config_t *baud_config)
{
    uart_baud_config_t low_speed;
    uart_baud_config_t high_speed;
    bool low_valid;
    bool high_valid;
    uint64_t low_error = 0ull;
    uint64_t high_error = 0ull;

    if ((baud_rate == 0u) || (baud_config == 0)) {
        return false;
    }

    low_valid = uart_brg_candidate(baud_rate, 0u, &low_speed, &low_error);
    high_valid = uart_brg_candidate(baud_rate, 1u, &high_speed, &high_error);

    if (!low_valid && !high_valid) {
        return false;
    }

    if (!low_valid || (high_valid && (high_error < low_error))) {
        *baud_config = high_speed;
        return high_error <= UART_BAUD_ERROR_PPM_MAX;
    }

    *baud_config = low_speed;
    return low_error <= UART_BAUD_ERROR_PPM_MAX;
}

static void uart_reset_buffers(uart_runtime_t *state)
{
    state->tx_head = 0u;
    state->tx_tail = 0u;
    state->rx_head = 0u;
    state->rx_tail = 0u;
    state->rx_overflow = false;
}

static void uart_disable(uart_t uart)
{
    switch (uart) {
    case UART_1:
        IEC0bits.U1RXIE = 0;
        IEC0bits.U1TXIE = 0;
        U1MODEbits.UARTEN = 0;
        U1STAbits.UTXEN = 0;
        IFS0bits.U1RXIF = 0;
        IFS0bits.U1TXIF = 0;
        break;

    case UART_2:
        IEC1bits.U2RXIE = 0;
        IEC1bits.U2TXIE = 0;
        U2MODEbits.UARTEN = 0;
        U2STAbits.UTXEN = 0;
        IFS1bits.U2RXIF = 0;
        IFS1bits.U2TXIF = 0;
        break;

    default:
        break;
    }
}

static void uart_enable_tx_interrupt(uart_t uart)
{
    switch (uart) {
    case UART_1:
        IFS0bits.U1TXIF = 1;
        IEC0bits.U1TXIE = 1;
        break;

    case UART_2:
        IFS1bits.U2TXIF = 1;
        IEC1bits.U2TXIE = 1;
        break;

    default:
        break;
    }
}

static void uart_disable_tx_interrupt(uart_t uart)
{
    switch (uart) {
    case UART_1:
        IEC0bits.U1TXIE = 0;
        IFS0bits.U1TXIF = 0;
        break;

    case UART_2:
        IEC1bits.U2TXIE = 0;
        IFS1bits.U2TXIF = 0;
        break;

    default:
        break;
    }
}

static void uart_hw_init(uart_t uart, const uart_baud_config_t *baud_config)
{
    switch (uart) {
    case UART_1:
        PMD1bits.U1MD = 0;
        U1MODE = 0;
        U1STA = 0;
        U1MODEbits.BRGH = baud_config->brgh;
        U1MODEbits.PDSEL = 0;
        U1MODEbits.STSEL = 0;
        U1BRG = baud_config->brg;
        U1STAbits.OERR = 0;
        U1STAbits.URXISEL = 0;
        U1STAbits.UTXISEL0 = 0;
        U1STAbits.UTXISEL1 = 0;
        IPC2bits.U1RXIP = UART_INTERRUPT_PRIORITY;
        IPC3bits.U1TXIP = UART_INTERRUPT_PRIORITY;
        IFS0bits.U1RXIF = 0;
        IFS0bits.U1TXIF = 0;
        IEC0bits.U1TXIE = 0;
        U1MODEbits.UARTEN = 1;
        U1STAbits.UTXEN = 1;
        IEC0bits.U1RXIE = 1;
        break;

    case UART_2:
        PMD1bits.U2MD = 0;
        U2MODE = 0;
        U2STA = 0;
        U2MODEbits.BRGH = baud_config->brgh;
        U2MODEbits.PDSEL = 0;
        U2MODEbits.STSEL = 0;
        U2BRG = baud_config->brg;
        U2STAbits.OERR = 0;
        U2STAbits.URXISEL = 0;
        U2STAbits.UTXISEL0 = 0;
        U2STAbits.UTXISEL1 = 0;
        IPC7bits.U2RXIP = UART_INTERRUPT_PRIORITY;
        IPC7bits.U2TXIP = UART_INTERRUPT_PRIORITY;
        IFS1bits.U2RXIF = 0;
        IFS1bits.U2TXIF = 0;
        IEC1bits.U2TXIE = 0;
        U2MODEbits.UARTEN = 1;
        U2STAbits.UTXEN = 1;
        IEC1bits.U2RXIE = 1;
        break;

    default:
        break;
    }
}

static void uart_rx_isr(uart_t uart)
{
    uart_runtime_t *state = &uart_state[uart];

    if (uart == UART_1) {
        if (U1STAbits.OERR != 0u) {
            U1STAbits.OERR = 0;
        }

        while (U1STAbits.URXDA != 0u) {
            bool rx_error = (U1STAbits.FERR != 0u) || (U1STAbits.PERR != 0u);
            uint8_t data = (uint8_t)U1RXREG;
            uint16_t next = ring_next(state->rx_head, UART_RX_BUFFER_SIZE);

            if (rx_error) {
                continue;
            }
            if (next == state->rx_tail) {
                state->rx_overflow = true;
                continue;
            }

            state->rx_buffer[state->rx_head] = data;
            state->rx_head = next;
        }

        IFS0bits.U1RXIF = 0;
    } else {
        if (U2STAbits.OERR != 0u) {
            U2STAbits.OERR = 0;
        }

        while (U2STAbits.URXDA != 0u) {
            bool rx_error = (U2STAbits.FERR != 0u) || (U2STAbits.PERR != 0u);
            uint8_t data = (uint8_t)U2RXREG;
            uint16_t next = ring_next(state->rx_head, UART_RX_BUFFER_SIZE);

            if (rx_error) {
                continue;
            }
            if (next == state->rx_tail) {
                state->rx_overflow = true;
                continue;
            }

            state->rx_buffer[state->rx_head] = data;
            state->rx_head = next;
        }

        IFS1bits.U2RXIF = 0;
    }
}

static void uart_tx_isr(uart_t uart)
{
    uart_runtime_t *state = &uart_state[uart];

    if (uart == UART_1) {
        while ((state->tx_tail != state->tx_head) && (U1STAbits.UTXBF == 0u)) {
            U1TXREG = state->tx_buffer[state->tx_tail];
            state->tx_tail = ring_next(state->tx_tail, UART_TX_BUFFER_SIZE);
        }

        if (state->tx_tail == state->tx_head) {
            uart_disable_tx_interrupt(UART_1);
        } else {
            IFS0bits.U1TXIF = 0;
        }
    } else {
        while ((state->tx_tail != state->tx_head) && (U2STAbits.UTXBF == 0u)) {
            U2TXREG = state->tx_buffer[state->tx_tail];
            state->tx_tail = ring_next(state->tx_tail, UART_TX_BUFFER_SIZE);
        }

        if (state->tx_tail == state->tx_head) {
            uart_disable_tx_interrupt(UART_2);
        } else {
            IFS1bits.U2TXIF = 0;
        }
    }
}

hal_status_t uartInit(uart_t uart, pin_t tx_pin, pin_t rx_pin, const uart_config_t *config)
{
    hal_status_t status;
    uart_baud_config_t baud_config;

    if (!uart_valid(uart) || (config == 0) || !uart_pins_valid(uart, tx_pin, rx_pin) || !uart_brg(config->baud_rate, &baud_config)) {
        return HAL_UNSUPPORTED;
    }

    uart_disable(uart);

    status = pinClaim(tx_pin, PIN_OWNER_UART);
    if (status != HAL_OK) {
        return status;
    }

    status = pinClaim(rx_pin, PIN_OWNER_UART);
    if (status != HAL_OK) {
        pinRelease(tx_pin, PIN_OWNER_UART);
        return status;
    }

    uart_reset_buffers(&uart_state[uart]);
    uart_state[uart].tx_pin = tx_pin;
    uart_state[uart].rx_pin = rx_pin;
    uart_state[uart].initialized = false;

    uart_configure_pins(uart);
    uart_hw_init(uart, &baud_config);

    uart_state[uart].initialized = true;
    return HAL_OK;
}

void uartDeinit(uart_t uart)
{
    uart_runtime_t *state;

    if (!uart_valid(uart)) {
        return;
    }

    state = &uart_state[uart];
    uart_disable(uart);

    if (state->initialized) {
        pinRelease(state->tx_pin, PIN_OWNER_UART);
        pinRelease(state->rx_pin, PIN_OWNER_UART);
    }

    uart_reset_buffers(state);
    state->tx_pin = (pin_t)0;
    state->rx_pin = (pin_t)0;
    state->initialized = false;
}

bool uartWriteByte(uart_t uart, uint8_t data)
{
    uart_runtime_t *state;
    uint16_t next;

    if (!uart_valid(uart) || !uart_state[uart].initialized) {
        return false;
    }

    state = &uart_state[uart];
    next = ring_next(state->tx_head, UART_TX_BUFFER_SIZE);
    if (next == state->tx_tail) {
        return false;
    }

    state->tx_buffer[state->tx_head] = data;
    state->tx_head = next;
    uart_enable_tx_interrupt(uart);

    return true;
}

uint16_t uartWrite(uart_t uart, const uint8_t *data, uint16_t length)
{
    uint16_t written = 0u;

    if (data == 0) {
        return 0u;
    }

    while ((written < length) && uartWriteByte(uart, data[written])) {
        written++;
    }

    return written;
}

uint16_t uartPrint(uart_t uart, const char *string)
{
    uint16_t written = 0u;

    if (string == 0) {
        return 0u;
    }

    while ((string[written] != '\0') && uartWriteByte(uart, (uint8_t)string[written])) {
        written++;
    }

    return written;
}

uint16_t uartTxAvailable(uart_t uart)
{
    if (!uart_valid(uart) || !uart_state[uart].initialized) {
        return 0u;
    }

    return (uint16_t)((UART_TX_BUFFER_SIZE - 1u) - ring_count(&uart_state[uart].tx_head, &uart_state[uart].tx_tail, UART_TX_BUFFER_SIZE));
}

bool uartTxBusy(uart_t uart)
{
    if (!uart_valid(uart) || !uart_state[uart].initialized) {
        return false;
    }

    if (uart_state[uart].tx_head != uart_state[uart].tx_tail) {
        return true;
    }

    switch (uart) {
    case UART_1:
        return U1STAbits.TRMT == 0u;

    case UART_2:
        return U2STAbits.TRMT == 0u;

    default:
        return false;
    }
}

uint16_t uartAvailable(uart_t uart)
{
    if (!uart_valid(uart) || !uart_state[uart].initialized) {
        return 0u;
    }

    return ring_count(&uart_state[uart].rx_head, &uart_state[uart].rx_tail, UART_RX_BUFFER_SIZE);
}

bool uartReadByte(uart_t uart, uint8_t *data)
{
    uart_runtime_t *state;

    if (!uart_valid(uart) || !uart_state[uart].initialized || (data == 0)) {
        return false;
    }

    state = &uart_state[uart];
    if (state->rx_tail == state->rx_head) {
        return false;
    }

    *data = state->rx_buffer[state->rx_tail];
    state->rx_tail = ring_next(state->rx_tail, UART_RX_BUFFER_SIZE);

    return true;
}

uint16_t uartRead(uart_t uart, uint8_t *data, uint16_t max_length)
{
    uint16_t read = 0u;

    if (data == 0) {
        return 0u;
    }

    while ((read < max_length) && uartReadByte(uart, &data[read])) {
        read++;
    }

    return read;
}

void uartFlushRx(uart_t uart)
{
    if (!uart_valid(uart)) {
        return;
    }

    uart_state[uart].rx_tail = uart_state[uart].rx_head;
}

bool uartRxOverflow(uart_t uart)
{
    if (!uart_valid(uart)) {
        return false;
    }

    return uart_state[uart].rx_overflow;
}

void uartClearRxOverflow(uart_t uart)
{
    if (!uart_valid(uart)) {
        return;
    }

    uart_state[uart].rx_overflow = false;
}

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    uart_rx_isr(UART_1);
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
    uart_tx_isr(UART_1);
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void)
{
    uart_rx_isr(UART_2);
}

void __attribute__((interrupt, no_auto_psv)) _U2TXInterrupt(void)
{
    uart_tx_isr(UART_2);
}
void reverse_string(char *buffer)
{
    uint16_t eos = 0;

    while (buffer[eos] != '\0') {
        eos++;
    }
    uint16_t limit = eos/2;
    for (uint16_t i = 0; i < limit; i++) {
        char temp = buffer[eos - 1 - i];
        buffer[eos - 1 - i] = buffer[i];
        buffer[i] = temp;
    }
}

void int_to_str(uint16_t value, char *buffer)
{
    int i = 0;

    do {
        buffer[i] = '0' + (value % 10);
        value /= 10;
        i++;
    } while (value > 0);

    buffer[i] = '\0';

    reverse_string(buffer);
}