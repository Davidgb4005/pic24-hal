#include "bare_uart.h"
#include "string.h"
#include "gpio.h"
#include "xc.h"
#pragma config FWDTEN = OFF
uint16_t reset_cause;

int main(void)
{
    RCONbits.SWDTEN = 0;
    pinMode(12, OUTPUT);
    char buffer[32];
    const char *my_word = "Hello World";
    strcpy(buffer, my_word);
    uart_init(9600);
    uart_send_string("UART ready\r\n");
    uint16_t msg_here = 0;
    int i = 0;

    // digitalWrite(12,1);
    while (1)
    {
        __builtin_clrwdt(); //<- kick dog
        if (uart_available)
        {
            char c = uart_recv_char(1);
            while ((c != '\r') && (c != '\n'))
            {
                buffer[i] = c;
                i++;
                c = uart_recv_char(1);
                digitalWrite(12, 1);
            }
            uart_send_char('\r');
            uart_send_char('\n');
            buffer[i] = '\0';
            msg_here = 1;
            i = 0;
        }

        if (msg_here)
        {
            if (!strcmp(buffer, "debug")){
                debug_print(1);
            }
            if (!strcmp(buffer, "password"))
            {
                uart_send_string("Correct Password\r\n");
            }
            else
            {
                uart_send_string("Incorrect Password\r\n");
            }
            uart_send_string(">");
            msg_here = 0;
            digitalWrite(12, 0);
        }
    }

    return 0;
}
