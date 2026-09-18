#ifndef PIC24_HAL_PIN_H
#define PIC24_HAL_PIN_H

#include <stdint.h>

#include "device.h"

typedef enum {
    PIN_1 = 1,
    PIN_2,
    PIN_3,
    PIN_4,
    PIN_5,
    PIN_6,
    PIN_7,
    PIN_8,
    PIN_9,
    PIN_10,
    PIN_11,
    PIN_12,
    PIN_13,
    PIN_14,
    PIN_15,
    PIN_16,
    PIN_17,
    PIN_18,
    PIN_19,
    PIN_20
} pin_t;

typedef enum {
    PIN_DIN,
    PIN_DIN_PULLUP,
    PIN_DOUT,
    PIN_DOUT_OPEN_DRAIN,
    PIN_AIN,
    PIN_PWM
} pin_mode_t;

typedef enum {
    LOW = 0,
    HIGH = 1
} pin_state_t;

typedef enum {
    PIN_OWNER_NONE,
    PIN_OWNER_GPIO,
    PIN_OWNER_ADC,
    PIN_OWNER_PWM,
    PIN_OWNER_UART,
    PIN_OWNER_I2C
} pin_owner_t;

hal_status_t pinClaim(pin_t pin, pin_owner_t owner);
void pinRelease(pin_t pin, pin_owner_t owner);

hal_status_t pinMode(pin_t pin, pin_mode_t mode);
hal_status_t digitalWrite(pin_t pin, pin_state_t state);
pin_state_t digitalRead(pin_t pin);
hal_status_t digitalToggle(pin_t pin);

uint16_t analogRead(pin_t pin);

hal_status_t pwmWrite(pin_t pin, uint16_t value);

#endif
