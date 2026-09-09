#include "assembly.h"
#include "uart.h"
#include "timer.h"
int main(){
    char my_array[50];
    uint8_t my_val = asm_adder();
    int_to_str(my_val,my_array);
    uart_config_t config = {
        .baud_rate = 9600
    };

    uartInit(UART_1, PIN_11, PIN_6, &config);
    uartPrint(UART_1, "UART ready\r\n");
    int_to_str(my_val,my_array);
    uartPrint(UART_1, (const char *)my_array);
    while(1){

    }

    return 0;
}

