#include "pin.h"
#include "timer.h"

#include <xc.h>

#define PIN_COUNT 21u
#define PIN_ADC_NONE 0xffu
#define PIN_PWM_NONE 0xffu
#define PIN_PWM_OC1 1u

typedef struct {
    volatile uint16_t *tris;
    volatile uint16_t *port;
    volatile uint16_t *lat;
    volatile uint16_t *pcfg;
    volatile uint16_t *odc;
    volatile uint16_t *cnpu;
    uint16_t mask;
    uint16_t pcfg_mask;
    uint16_t cnpu_mask;
    uint8_t adc_channel;
    uint8_t pwm_channel;
} pin_hw_t;

typedef struct {
    pin_owner_t owner;
    pin_mode_t mode;
} pin_runtime_t;

#define PIN_A(bit) \
    .tris = &TRISA, .port = &PORTA, .lat = &LATA, .odc = &ODCA, .mask = (uint16_t)(1u << (bit))

#define PIN_B(bit) \
    .tris = &TRISB, .port = &PORTB, .lat = &LATB, .odc = &ODCB, .mask = (uint16_t)(1u << (bit))

#define PIN_ANALOG(channel) \
    .pcfg = &AD1PCFG, .pcfg_mask = (uint16_t)(1u << (channel)), .adc_channel = (channel)

#define PIN_CN1(cn) \
    .cnpu = &CNPU1, .cnpu_mask = (uint16_t)(1u << (cn))

#define PIN_CN2(bit) \
    .cnpu = &CNPU2, .cnpu_mask = (uint16_t)(1u << (bit))

static const pin_hw_t pin_hw[PIN_COUNT] = {
    [PIN_1] = { PIN_A(5), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_2] = { PIN_A(0), PIN_ANALOG(0), PIN_CN1(2), .pwm_channel = PIN_PWM_NONE },
    [PIN_3] = { PIN_A(1), PIN_ANALOG(1), PIN_CN1(3), .pwm_channel = PIN_PWM_NONE },
    [PIN_4] = { PIN_B(0), PIN_ANALOG(2), PIN_CN1(4), .pwm_channel = PIN_PWM_NONE },
    [PIN_5] = { PIN_B(1), PIN_ANALOG(3), PIN_CN1(5), .pwm_channel = PIN_PWM_NONE },
    [PIN_6] = { PIN_B(2), PIN_CN1(6), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_7] = { PIN_A(2), PIN_ANALOG(4), PIN_CN2(14), .pwm_channel = PIN_PWM_NONE },
    [PIN_8] = { PIN_A(3), PIN_ANALOG(5), PIN_CN2(13), .pwm_channel = PIN_PWM_NONE },
    [PIN_9] = { PIN_B(4), PIN_CN1(1), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_10] = { PIN_A(4), PIN_CN1(0), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_11] = { PIN_B(7), PIN_CN2(7), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_12] = { PIN_B(8), PIN_CN2(6), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_13] = { PIN_B(9), PIN_CN2(5), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_14] = { PIN_A(6), PIN_CN1(8), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_OC1 },
    [PIN_15] = { PIN_B(12), PIN_ANALOG(12), PIN_CN1(14), .pwm_channel = PIN_PWM_NONE },
    [PIN_16] = { PIN_B(13), PIN_ANALOG(11), PIN_CN1(13), .pwm_channel = PIN_PWM_NONE },
    [PIN_17] = { PIN_B(14), PIN_ANALOG(10), PIN_CN1(12), .pwm_channel = PIN_PWM_NONE },
    [PIN_18] = { PIN_B(15), PIN_CN1(11), .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_19] = { .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE },
    [PIN_20] = { .adc_channel = PIN_ADC_NONE, .pwm_channel = PIN_PWM_NONE }
};

static pin_runtime_t pin_state[PIN_COUNT];
static uint16_t pwm_oc1_value;

static void write_bit(volatile uint16_t *reg, uint16_t mask, uint8_t value)
{
    if (value != 0u) {
        *reg |= mask;
    } else {
        *reg &= (uint16_t)~mask;
    }
}

static void configure_digital(const pin_hw_t *hw)
{
    if (hw->pcfg != 0) {
        *hw->pcfg |= hw->pcfg_mask;
    }
}

static hal_status_t configure_pwm_pin(const pin_hw_t *hw)
{
    if (hw->pwm_channel != PIN_PWM_OC1) {
        return HAL_UNSUPPORTED;
    }

    PMD2bits.OC1MD = 0;
    configure_digital(hw);
    write_bit(hw->odc, hw->mask, 0u);
    write_bit(hw->tris, hw->mask, 0u);
    PADCFG1bits.OC1TRIS = 0;
    OC1CONbits.OCM = 0;
    OC1CONbits.OCTSEL = 0;
    OC1R = 0;
    OC1RS = 0;
    OC1CONbits.OCM = 6;
    return HAL_OK;
}

static void pwm_oc1_apply(void)
{
    uint16_t period = pwmGetPeriod();
    uint32_t duty = (((uint32_t)period + 1u) * pwm_oc1_value) / 65535u;

    if ((duty > period) && (period != 0xffffu)) {
        duty = period;
    }

    OC1RS = (uint16_t)duty;
}

void pinPwmTimebaseChanged(void)
{
    if ((pin_state[PIN_14].owner == PIN_OWNER_PWM) && (pin_state[PIN_14].mode == PIN_PWM)) {
        pwm_oc1_apply();
    }
}

hal_status_t pinClaim(pin_t pin, pin_owner_t owner)
{
    pin_runtime_t *state = &pin_state[pin];

    if ((state->owner == PIN_OWNER_NONE) || (state->owner == owner)) {
        state->owner = owner;
        return HAL_OK;
    }

    return HAL_BUSY;
}

void pinRelease(pin_t pin, pin_owner_t owner)
{
    if (pin_state[pin].owner == owner) {
        pin_state[pin].owner = PIN_OWNER_NONE;
    }
}

hal_status_t pinMode(pin_t pin, pin_mode_t mode)
{
    const pin_hw_t *hw = &pin_hw[pin];
    pin_owner_t owner = PIN_OWNER_GPIO;
    hal_status_t status;

    if (mode == PIN_AIN) {
        owner = PIN_OWNER_ADC;
    } else if (mode == PIN_PWM) {
        owner = PIN_OWNER_PWM;
    }

    status = pinClaim(pin, owner);
    if (status != HAL_OK) {
        return status;
    }

    if (hw->cnpu != 0) {
        *hw->cnpu &= (uint16_t)~hw->cnpu_mask;
    }
    if (hw->odc != 0) {
        write_bit(hw->odc, hw->mask, 0u);
    }

    switch (mode) {
    case PIN_DIN:
        configure_digital(hw);
        write_bit(hw->tris, hw->mask, 1u);
        break;

    case PIN_DIN_PULLUP:
        configure_digital(hw);
        write_bit(hw->tris, hw->mask, 1u);
        if (hw->cnpu != 0) {
            *hw->cnpu |= hw->cnpu_mask;
        }
        break;

    case PIN_DOUT:
        configure_digital(hw);
        write_bit(hw->tris, hw->mask, 0u);
        break;

    case PIN_DOUT_OPEN_DRAIN:
        configure_digital(hw);
        write_bit(hw->odc, hw->mask, 1u);
        write_bit(hw->tris, hw->mask, 0u);
        break;

    case PIN_AIN:
        if (hw->pcfg == 0) {
            pinRelease(pin, owner);
            return HAL_UNSUPPORTED;
        }
        write_bit(hw->tris, hw->mask, 1u);
        *hw->pcfg &= (uint16_t)~hw->pcfg_mask;
        break;

    case PIN_PWM:
        status = configure_pwm_pin(hw);
        if (status != HAL_OK) {
            pinRelease(pin, owner);
            return status;
        }
        break;

    default:
        pinRelease(pin, owner);
        return HAL_ERROR;
    }

    pin_state[pin].mode = mode;
    return HAL_OK;
}

hal_status_t digitalWrite(pin_t pin, pin_state_t state)
{
    const pin_hw_t *hw = &pin_hw[pin];
    hal_status_t status = pinClaim(pin, PIN_OWNER_GPIO);

    if (status != HAL_OK) {
        return status;
    }

    write_bit(hw->lat, hw->mask, (uint8_t)state);
    return HAL_OK;
}

pin_state_t digitalRead(pin_t pin)
{
    const pin_hw_t *hw = &pin_hw[pin];

    return ((*hw->port & hw->mask) != 0u) ? HIGH : LOW;
}

hal_status_t digitalToggle(pin_t pin)
{
    const pin_hw_t *hw = &pin_hw[pin];
    hal_status_t status = pinClaim(pin, PIN_OWNER_GPIO);

    if (status != HAL_OK) {
        return status;
    }

    *hw->lat ^= hw->mask;
    return HAL_OK;
}

uint16_t analogRead(pin_t pin)
{
    const pin_hw_t *hw = &pin_hw[pin];

    if ((hw->pcfg == 0) || (pinClaim(pin, PIN_OWNER_ADC) != HAL_OK)) {
        return 0u;
    }

    PMD1bits.ADC1MD = 0;
    write_bit(hw->tris, hw->mask, 1u);
    *hw->pcfg &= (uint16_t)~hw->pcfg_mask;

    AD1CON1bits.ADON = 0;
    AD1CON1bits.FORM = 0;
    AD1CON1bits.SSRC = 7;
    AD1CON1bits.ASAM = 0;
    AD1CON2 = 0;
    AD1CON3bits.ADRC = 0;
    AD1CON3bits.SAMC = 16;
    AD1CON3bits.ADCS = 2;
    AD1CHSbits.CH0NA = 0;
    AD1CHSbits.CH0SA = hw->adc_channel;
    AD1CON1bits.ADON = 1;

    AD1CON1bits.DONE = 0;
    AD1CON1bits.SAMP = 1;
    while (AD1CON1bits.DONE == 0) {
    }

    return ADC1BUF0;
}

hal_status_t pwmWrite(pin_t pin, uint16_t value)
{
    const pin_hw_t *hw = &pin_hw[pin];
    hal_status_t status;

    if (pin_state[pin].mode != PIN_PWM) {
        return HAL_UNSUPPORTED;
    }
    if ((pin_state[pin].mode == PIN_PWM) && (hw->pwm_channel != PIN_PWM_OC1)) {
        return HAL_UNSUPPORTED;
    }

    status = pinClaim(pin, PIN_OWNER_PWM);
    if (status != HAL_OK) {
        return status;
    }

    pwm_oc1_value = value;
    pwm_oc1_apply();
    return HAL_OK;
}
