

#include "scheduler.h"
#include "stdint.h"
#define MAX_TASKS 8
#define PIC24F16KA101 0
#define NO_ACTIVE_TASK UINT16_MAX


uint16_t controller = PIC24F16KA101;
task_handler_t *tasks[MAX_TASKS];

uint16_t task_count = 0;
uint16_t task_index = NO_ACTIVE_TASK;



void register_task(task_handler_t *task,void (*entry)(void *),void *parameters,uint16_t *stack,uint16_t stack_size)
{
    uint16_t *sp = stack;

    if (task_count >= MAX_TASKS) {
        return;
    }

    task->stack = stack;
    task->stack_size = stack_size;

    *sp++ = (uint16_t)entry;
    *sp++ = 0x0000;

    *sp++ = (uint16_t)parameters;

    for (int i = 1; i <= 14; i++) {
        *sp++ = 0;
    }

    task->sp = sp;

    tasks[task_count] = task;
    task_count++;
}


uint16_t *next_task(uint16_t *current_sp)
{
    if (task_count == 0u) {
        return current_sp;
    }

    if (task_index != NO_ACTIVE_TASK) {
        tasks[task_index]->sp = current_sp;
    }

    task_index++;
    if (task_index >= task_count) {
        task_index = 0;
    }

    return tasks[task_index]->sp;
}
