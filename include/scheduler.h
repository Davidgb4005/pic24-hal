#ifndef PIC24_HAL_SCHEDULER_H
#define PIC24_HAL_SCHEDULER_H

#include "stdint.h"

typedef struct {
    uint16_t *sp;
    uint16_t *stack;
    uint16_t stack_size;
} task_handler_t;

void register_task(
    task_handler_t *task,
    void (*entry)(void *),
    void *parameters,
    uint16_t *stack,
    uint16_t stack_size
);

uint16_t *next_task(uint16_t *current_sp);

#endif
