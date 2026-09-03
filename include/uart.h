#ifndef PIC24_HAL_UART_H
#define PIC24_HAL_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "pin.h"

#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE 64u
#endif

#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE 64u
#endif

typedef enum {
    UART_1,
    UART_2,

    UART_COUNT
} uart_t;

typedef struct {
    uint32_t baud_rate;
} uart_config_t;

hal_status_t uartInit(uart_t uart, pin_t tx_pin, pin_t rx_pin, const uart_config_t *config);
void uartDeinit(uart_t uart);

bool uartWriteByte(uart_t uart, uint8_t data);
uint16_t uartWrite(uart_t uart, const uint8_t *data, uint16_t length);
uint16_t uartPrint(uart_t uart, const char *string);
uint16_t uartTxAvailable(uart_t uart);
bool uartTxBusy(uart_t uart);

uint16_t uartAvailable(uart_t uart);
bool uartReadByte(uart_t uart, uint8_t *data);
uint16_t uartRead(uart_t uart, uint8_t *data, uint16_t max_length);
void uartFlushRx(uart_t uart);
bool uartRxOverflow(uart_t uart);
void uartClearRxOverflow(uart_t uart);

#endif
