#include <stdint.h>
#include <xc.h>

#include "bare_uart.h"
#include "device.h"

void uart_init(uint32_t baud_rate)
{
    uint32_t brg;

    TRISBbits.TRISB7 = 0;
    TRISBbits.TRISB2 = 1;

    U1MODE = 0;
    U1STA = 0;

    brg = (HAL_FCY / (16UL * baud_rate)) - 1UL;
    U1BRG = (uint16_t)brg;

    U1MODEbits.BRGH = 0;
    U1MODEbits.PDSEL = 0;
    U1MODEbits.STSEL = 0;
    U1STAbits.OERR = 0;

    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;
}

void uart_send_char(char c)
{
    while (U1STAbits.UTXBF != 0) {
    }

    U1TXREG = (uint8_t)c;
}

void uart_send_string(const char *string)
{
    while (*string != '\0') {
        uart_send_char(*string);
        string++;
    }
}

char uart_recv_char(uint16_t echo)//Always call uart_avail first this does not check buffer state if called early -> UB
{
    while (U1STAbits.URXDA == 0){
    if (U1STAbits.OERR != 0) {
        U1STAbits.OERR = 0;//Fuck about with overflow handling later
    }}
    char c = (char)U1RXREG;
    if (echo){
        uart_send_char(c);
    }

    return c;
}

uint16_t uart_available(){
    if (U1STAbits.URXDA != 0){
        return 1;
    }
    else{
        return 0;
    }
}


uint16_t uart_recv_string(char *buffer, uint16_t max_length)
{
    uint16_t length = 0;

    if (max_length == 0) {
        return 0;
    }

    while (length < (uint16_t)(max_length - 1U)) {
        char c = uart_recv_char(0);
        if ((c == '\r') || (c == '\n')) {
            break;
        }
    buffer[length] = c;
    length++;
    

    }

    buffer[length] = '\0';
    return length;
}

void debug_print(uint16_t reset){
    uart_send_string("Printing RCON log\r\n");
        if (RCONbits.TRAPR)
        {
            uart_send_string(" TRAPR ");
        }
        if (RCONbits.IOPUWR)
        {
            uart_send_string(" IOPUWR ");
        }
        if (RCONbits.DPSLP)
        {
            uart_send_string(" DPSLP ");
        }
        if (RCONbits.EXTR)
        {
            uart_send_string(" EXTR ");
        }
        if (RCONbits.SWR)
        {
            uart_send_string(" SWR ");
        }
        if (RCONbits.WDTO)
        {
            uart_send_string(" WDTO ");
        }
        if (RCONbits.SLEEP)
        {
            uart_send_string(" SLEEP ");
        }
        if (RCONbits.IDLE)
        {
            uart_send_string(" IDLE ");
        }
        if (RCONbits.BOR)
        {
            uart_send_string(" BOR ");
        }
        if (RCONbits.POR)
        {
            uart_send_string(" POR ");
        }
    if (reset){
        uart_send_string("\r\nReseting RCON bits\r\n");
        RCONbits.TRAPR  = 0;
        RCONbits.IOPUWR = 0;
        RCONbits.DPSLP  = 0;
        RCONbits.EXTR   = 0;
        RCONbits.SWR    = 0;
        RCONbits.WDTO   = 0;
        RCONbits.SLEEP  = 0;
        RCONbits.IDLE   = 0;
        RCONbits.BOR    = 0;
        RCONbits.POR    = 0;
    }
    uart_send_string("Printing INTCON log\r\n");
    if (INTCON1bits.MATHERR)
    {
        uart_send_string(" MATHERR ");
    }

    if (INTCON1bits.ADDRERR)
    {
        uart_send_string(" ADDRERR ");
    }

    if (INTCON1bits.STKERR)
    {
        uart_send_string(" STKERR ");
    }

    if (INTCON1bits.OSCFAIL)
    {
        uart_send_string(" OSCFAIL ");
    }
    if (reset){
        uart_send_string("\r\nReseting INTCON bits\r\n");
        INTCON1bits.MATHERR = 0;
        INTCON1bits.ADDRERR = 0;
        INTCON1bits.STKERR  = 0;
        INTCON1bits.OSCFAIL = 0;
    }
    uart_send_string("\r\nEnd Of Debug\r\n");

}
