#ifndef PIC24_HAL_TIMER_H
#define PIC24_HAL_TIMER_H

#include <stdint.h>

#include "device.h"

hal_status_t timerTimebaseInit(void);

uint32_t millis(void);
uint32_t micros(void);

hal_status_t pwmFreq(uint32_t frequency_hz);
uint16_t pwmGetPeriod(void);

#endif
