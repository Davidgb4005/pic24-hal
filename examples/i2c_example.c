#include "i2c.h"
#include "uart.h"

static void print_hex_nibble(uint8_t value)
{
    value &= 0x0fu;
    uartWriteByte(UART_1, (uint8_t)(value < 10u ? ('0' + value) : ('A' + value - 10u)));
}

static void print_hex_byte(uint8_t value)
{
    print_hex_nibble((uint8_t)(value >> 4));
    print_hex_nibble(value);
}

int main(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600
    };

    i2c_config_t i2c_config = {
        .clock_hz = 100000
    };

    uartInit(UART_1, PIN_11, PIN_6, &uart_config);
    i2cInit(I2C_1, PIN_12, PIN_13, &i2c_config);

    uartPrint(UART_1, "I2C scan\r\n");

    while (1) {
        uint8_t address;

        for (address = 0x08u; address < 0x78u; address++) {
            if (i2cReady(I2C_1, address)) {
                uartPrint(UART_1, "Found 0x");
                print_hex_byte(address);
                uartPrint(UART_1, "\r\n");
            }
        }

        while (uartTxBusy(UART_1)) {
        }
    }

    return 0;
}
