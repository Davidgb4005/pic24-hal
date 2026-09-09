#include "i2c.h"

#include <xc.h>

#define I2C_TIMEOUT_COUNT 60000u

typedef struct {
    pin_t scl_pin;
    pin_t sda_pin;
    bool initialized;
} i2c_runtime_t;

static i2c_runtime_t i2c_state[I2C_COUNT];

static bool i2c_valid(i2c_t i2c)
{
    return i2c < I2C_COUNT;
}

static bool i2c_pins_valid(i2c_t i2c, pin_t scl_pin, pin_t sda_pin)
{
    switch (i2c) {
    case I2C_1:
        return (scl_pin == PIN_12) && (sda_pin == PIN_13);

    default:
        return false;
    }
}

static bool i2c_brg(uint32_t clock_hz, uint16_t *brg)
{
    uint32_t value;
    uint32_t delay;

    if ((clock_hz == 0u) || (brg == 0)) {
        return false;
    }

    value = (uint32_t)(((uint64_t)HAL_FCY + (clock_hz / 2ull)) / clock_hz);
    delay = HAL_FCY / 10000000ul;

    if (value <= delay) {
        return false;
    }

    value -= delay;
    value--;
    if (value > 0x01ffu) {
        return false;
    }

    *brg = (uint16_t)value;
    return true;
}

static bool i2c_wait_control_clear(void)
{
    uint16_t timeout = I2C_TIMEOUT_COUNT;

    while ((I2C1CONbits.SEN != 0u) ||
           (I2C1CONbits.RSEN != 0u) ||
           (I2C1CONbits.PEN != 0u) ||
           (I2C1CONbits.RCEN != 0u) ||
           (I2C1CONbits.ACKEN != 0u)) {
        if (timeout == 0u) {
            return false;
        }
        timeout--;
    }

    return true;
}

static bool i2c_wait_trstat_clear(void)
{
    uint16_t timeout = I2C_TIMEOUT_COUNT;

    while (I2C1STATbits.TRSTAT != 0u) {
        if (timeout == 0u) {
            return false;
        }
        timeout--;
    }

    return true;
}

static bool i2c_wait_rbf_set(void)
{
    uint16_t timeout = I2C_TIMEOUT_COUNT;

    while (I2C1STATbits.RBF == 0u) {
        if (timeout == 0u) {
            return false;
        }
        timeout--;
    }

    return true;
}

static hal_status_t i2c_start(void)
{
    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.SEN = 1;

    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    if (I2C1STATbits.BCL != 0u) {
        I2C1STATbits.BCL = 0;
        return HAL_ERROR;
    }

    return HAL_OK;
}

static hal_status_t i2c_restart(void)
{
    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.RSEN = 1;

    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    if (I2C1STATbits.BCL != 0u) {
        I2C1STATbits.BCL = 0;
        return HAL_ERROR;
    }

    return HAL_OK;
}

static void i2c_stop(void)
{
    if (i2c_wait_control_clear()) {
        IFS1bits.MI2C1IF = 0;
        I2C1CONbits.PEN = 1;
        (void)i2c_wait_control_clear();
    }
}

static hal_status_t i2c_write_raw(uint8_t data)
{
    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    I2C1STATbits.IWCOL = 0;
    IFS1bits.MI2C1IF = 0;
    I2C1TRN = data;

    if (I2C1STATbits.IWCOL != 0u) {
        I2C1STATbits.IWCOL = 0;
        return HAL_BUSY;
    }

    if (!i2c_wait_trstat_clear()) {
        return HAL_BUSY;
    }

    if (I2C1STATbits.BCL != 0u) {
        I2C1STATbits.BCL = 0;
        return HAL_ERROR;
    }

    if (I2C1STATbits.ACKSTAT != 0u) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static hal_status_t i2c_read_raw(uint8_t *data, bool ack)
{
    if ((data == 0) || !i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.RCEN = 1;

    if (!i2c_wait_rbf_set()) {
        return HAL_BUSY;
    }

    *data = (uint8_t)I2C1RCV;

    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    I2C1CONbits.ACKDT = ack ? 0u : 1u;
    I2C1CONbits.ACKEN = 1;

    if (!i2c_wait_control_clear()) {
        return HAL_BUSY;
    }

    return HAL_OK;
}

static hal_status_t i2c_address(uint8_t address, bool read)
{
    return i2c_write_raw((uint8_t)((address << 1) | (read ? 1u : 0u)));
}

hal_status_t i2cInit(i2c_t i2c, pin_t scl_pin, pin_t sda_pin, const i2c_config_t *config)
{
    hal_status_t status;
    uint16_t brg;

    if (!i2c_valid(i2c) || (config == 0) || !i2c_pins_valid(i2c, scl_pin, sda_pin) || !i2c_brg(config->clock_hz, &brg)) {
        return HAL_UNSUPPORTED;
    }

    i2cDeinit(i2c);

    status = pinClaim(scl_pin, PIN_OWNER_I2C);
    if (status != HAL_OK) {
        return status;
    }

    status = pinClaim(sda_pin, PIN_OWNER_I2C);
    if (status != HAL_OK) {
        pinRelease(scl_pin, PIN_OWNER_I2C);
        return status;
    }

    PMD1bits.I2C1MD = 0;

    LATBbits.LATB8 = 1;
    LATBbits.LATB9 = 1;
    ODCBbits.ODB8 = 1;
    ODCBbits.ODB9 = 1;
    TRISBbits.TRISB8 = 1;
    TRISBbits.TRISB9 = 1;

    IEC1bits.MI2C1IE = 0;
    IEC1bits.SI2C1IE = 0;
    IFS1bits.MI2C1IF = 0;
    IFS1bits.SI2C1IF = 0;

    I2C1CONbits.I2CEN = 0;
    I2C1CON = 0;
    I2C1STAT = 0;
    I2C1BRG = brg;
    I2C1CONbits.DISSLW = (config->clock_hz <= 100000u) ? 1u : 0u;
    I2C1CONbits.I2CEN = 1;

    i2c_state[i2c].scl_pin = scl_pin;
    i2c_state[i2c].sda_pin = sda_pin;
    i2c_state[i2c].initialized = true;

    return HAL_OK;
}

void i2cDeinit(i2c_t i2c)
{
    i2c_runtime_t *state;

    if (!i2c_valid(i2c)) {
        return;
    }

    state = &i2c_state[i2c];

    IEC1bits.MI2C1IE = 0;
    IEC1bits.SI2C1IE = 0;
    IFS1bits.MI2C1IF = 0;
    IFS1bits.SI2C1IF = 0;
    I2C1CONbits.I2CEN = 0;

    if (state->initialized) {
        pinRelease(state->scl_pin, PIN_OWNER_I2C);
        pinRelease(state->sda_pin, PIN_OWNER_I2C);
    }

    state->scl_pin = (pin_t)0;
    state->sda_pin = (pin_t)0;
    state->initialized = false;
}

hal_status_t i2cWrite(i2c_t i2c, uint8_t address, const uint8_t *data, uint16_t length)
{
    hal_status_t status;
    uint16_t i;

    if (!i2c_valid(i2c) || !i2c_state[i2c].initialized || ((data == 0) && (length != 0u)) || (address > 0x7fu)) {
        return HAL_UNSUPPORTED;
    }

    status = i2c_start();
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    status = i2c_address(address, false);
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    for (i = 0u; i < length; i++) {
        status = i2c_write_raw(data[i]);
        if (status != HAL_OK) {
            i2c_stop();
            return status;
        }
    }

    i2c_stop();
    return HAL_OK;
}

hal_status_t i2cRead(i2c_t i2c, uint8_t address, uint8_t *data, uint16_t length)
{
    hal_status_t status;
    uint16_t i;

    if (!i2c_valid(i2c) || !i2c_state[i2c].initialized || ((data == 0) && (length != 0u)) || (address > 0x7fu)) {
        return HAL_UNSUPPORTED;
    }

    status = i2c_start();
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    status = i2c_address(address, true);
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    for (i = 0u; i < length; i++) {
        status = i2c_read_raw(&data[i], i < (uint16_t)(length - 1u));
        if (status != HAL_OK) {
            i2c_stop();
            return status;
        }
    }

    i2c_stop();
    return HAL_OK;
}

hal_status_t i2cWriteRead(i2c_t i2c, uint8_t address, const uint8_t *tx_data, uint16_t tx_length, uint8_t *rx_data, uint16_t rx_length)
{
    hal_status_t status;
    uint16_t i;

    if (!i2c_valid(i2c) ||
        !i2c_state[i2c].initialized ||
        ((tx_data == 0) && (tx_length != 0u)) ||
        ((rx_data == 0) && (rx_length != 0u)) ||
        (address > 0x7fu)) {
        return HAL_UNSUPPORTED;
    }

    if (tx_length == 0u) {
        return i2cRead(i2c, address, rx_data, rx_length);
    }

    if (rx_length == 0u) {
        return i2cWrite(i2c, address, tx_data, tx_length);
    }

    status = i2c_start();
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    status = i2c_address(address, false);
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    for (i = 0u; i < tx_length; i++) {
        status = i2c_write_raw(tx_data[i]);
        if (status != HAL_OK) {
            i2c_stop();
            return status;
        }
    }

    status = i2c_restart();
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    status = i2c_address(address, true);
    if (status != HAL_OK) {
        i2c_stop();
        return status;
    }

    for (i = 0u; i < rx_length; i++) {
        status = i2c_read_raw(&rx_data[i], i < (uint16_t)(rx_length - 1u));
        if (status != HAL_OK) {
            i2c_stop();
            return status;
        }
    }

    i2c_stop();
    return HAL_OK;
}

bool i2cReady(i2c_t i2c, uint8_t address)
{
    hal_status_t status;

    if (!i2c_valid(i2c) || !i2c_state[i2c].initialized || (address > 0x7fu)) {
        return false;
    }

    status = i2c_start();
    if (status == HAL_OK) {
        status = i2c_address(address, false);
    }
    i2c_stop();

    return status == HAL_OK;
}
