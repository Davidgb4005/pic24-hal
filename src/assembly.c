#include "assembly.h"

#include <stdint.h>

extern uint16_t asm_add(uint16_t a, uint16_t b);

int asm_adder(void)
{
    volatile uint16_t result;
    char my_array[50];
    result = asm_add(40, 20);

    return result;
}