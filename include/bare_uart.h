#ifndef BARE_UART_H
#define BARE_UART_H

#include <stdint.h>

void uart_init(uint32_t baud_rate);
void uart_send_char(char c);
void uart_send_string(const char *string);
char uart_recv_char(uint16_t echo);
uint16_t uart_recv_string(char *buffer, uint16_t max_length);
uint16_t uart_available();
void debug_print(uint16_t reset);
#endif
