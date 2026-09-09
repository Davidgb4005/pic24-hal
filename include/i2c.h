#ifndef PIC24_HAL_I2C_H
#define PIC24_HAL_I2C_H

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "pin.h"

typedef enum {
    I2C_1,

    I2C_COUNT
} i2c_t;

typedef struct {
    uint32_t clock_hz;
} i2c_config_t;

hal_status_t i2cInit(i2c_t i2c, pin_t scl_pin, pin_t sda_pin, const i2c_config_t *config);
void i2cDeinit(i2c_t i2c);

hal_status_t i2cWrite(i2c_t i2c, uint8_t address, const uint8_t *data, uint16_t length);
hal_status_t i2cRead(i2c_t i2c, uint8_t address, uint8_t *data, uint16_t length);
hal_status_t i2cWriteRead(i2c_t i2c, uint8_t address, const uint8_t *tx_data, uint16_t tx_length, uint8_t *rx_data, uint16_t rx_length);

bool i2cReady(i2c_t i2c, uint8_t address);

#endif
