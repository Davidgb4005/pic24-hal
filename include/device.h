#ifndef PIC24_HAL_DEVICE_H
#define PIC24_HAL_DEVICE_H

#include <stdint.h>

#ifndef HAL_FCY
#define HAL_FCY 2000000UL
#endif

typedef enum {
    HAL_OK = 0,
    HAL_ERROR,
    HAL_BUSY,
    HAL_UNSUPPORTED
} hal_status_t;

#endif
