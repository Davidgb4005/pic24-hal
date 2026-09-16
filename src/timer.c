#include "timer.h"

#include <xc.h>

#define TIMER_PERIOD_MAX 0xffffu

typedef struct {
    uint16_t divisor;
    uint8_t bits;
} timer_prescaler_hw_t;

static const timer_prescaler_hw_t timer_prescalers[] = {
    { 1u, 0u },
    { 8u, 1u },
    { 64u, 2u },
    { 256u, 3u }
};

static volatile uint32_t timer1_periods;
static uint16_t timer1_period = TIMER_PERIOD_MAX;
static uint16_t timer1_prescaler = 1u;
static uint16_t pwm_period = TIMER_PERIOD_MAX;

extern void pinPwmTimebaseChanged(void);

static uint32_t timer_ticks_to_us(uint64_t ticks, uint16_t prescaler)
{
    return (uint32_t)((ticks * (uint64_t)prescaler * 1000000ull) / (uint64_t)HAL_FCY);
}

static uint64_t timer1_ticks_snapshot(void)
{
    uint32_t periods;
    uint16_t count;
    uint8_t t1ie;

    t1ie = IEC0bits.T1IE;
    IEC0bits.T1IE = 0;

    periods = timer1_periods;
    count = TMR1;

    if (IFS0bits.T1IF != 0u) {
        periods++;
        count = TMR1;
    }

    IEC0bits.T1IE = t1ie;

    return ((uint64_t)periods * ((uint64_t)timer1_period + 1u)) + count;
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    timer1_periods++;
    IFS0bits.T1IF = 0;
}

hal_status_t timerTimebaseInit(void)
{
    PMD1bits.T1MD = 0;

    T1CONbits.TON = 0;
    T1CONbits.TCS = 0;
    T1CONbits.TSYNC = 0;
    T1CONbits.TGATE = 0;
    T1CONbits.TCKPS = 0;

    timer1_periods = 0;
    timer1_period = TIMER_PERIOD_MAX;
    timer1_prescaler = 1u;

    TMR1 = 0;
    PR1 = timer1_period;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;

    return HAL_OK;
}

uint32_t millis(void)
{
    return micros() / 1000u;
}

uint32_t micros(void)
{
    return timer_ticks_to_us(timer1_ticks_snapshot(), timer1_prescaler);
}

hal_status_t pwmFreq(uint32_t frequency_hz)
{
    uint8_t index;

    if (frequency_hz == 0u) {
        return HAL_UNSUPPORTED;
    }

    for (index = 0u; index < (uint8_t)(sizeof(timer_prescalers) / sizeof(timer_prescalers[0])); index++) {
        uint32_t ticks = (uint32_t)(HAL_FCY / ((uint64_t)timer_prescalers[index].divisor * frequency_hz));

        if ((ticks > 0u) && (ticks <= ((uint32_t)TIMER_PERIOD_MAX + 1u))) {
            pwm_period = (uint16_t)(ticks - 1u);

            PMD1bits.T2MD = 0;
            T2CONbits.TON = 0;
            T2CONbits.TCS = 0;
            T2CONbits.T32 = 0;
            T2CONbits.TGATE = 0;
            T2CONbits.TCKPS = timer_prescalers[index].bits;
            PR2 = pwm_period;
            TMR2 = 0;
            IFS0bits.T2IF = 0;
            IEC0bits.T2IE = 0;
            T2CONbits.TON = 1;

            return HAL_OK;
        }
    }

    return HAL_UNSUPPORTED;
}

uint16_t pwmGetPeriod(void)
{
    return pwm_period;
}
