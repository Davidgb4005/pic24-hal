#include "uart.h"
#include "timer.h"
#include "pin.h"
int main(void)
{
    uart_config_t config = {
        .baud_rate = 9600
    };

    uartInit(UART_1, PIN_11, PIN_6, &config);
    uartPrint(UART_1, "UART ready\r\n");
    pinMode(PIN_2,PIN_DIN_PULLUP);
int ons = 0;
    while (1) {
        uint8_t byte;
	if (digitalRead(PIN_2) && !ons){
		uartPrint(UART_1,"BUTTON PRESSED\r\n");
		ons = 1;
	}
	
	else if (!digitalRead(PIN_2) && ons){
		ons = 0;
	}

        if (uartReadByte(UART_1, &byte)) {
            uartWriteByte(UART_1, byte+1);
        }
    }

    return 0;
}
