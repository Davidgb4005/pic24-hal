# PIC24F16KA101 Timer Module — Final Scope

## Hardware Timer Allocation

Use fixed timer assignments:

```text
Timer1 -> system timebase for millis() and micros()
Timer2 -> shared PWM timebase for all PWM outputs
Timer3 -> unused / reserved for future use
```

Do not configure or expose Timer3 in the initial implementation.

Do not implement timer ownership, automatic timer allocation, or a general-purpose timer API.

---

# Timer1 — System Timebase

Timer1 is dedicated permanently to general elapsed-time measurement.

Provide:

```c
hal_status_t timerTimebaseInit(void);

uint32_t millis(void);
uint32_t micros(void);
```

`timerTimebaseInit()` should:

1. Configure Timer1 using the system instruction-cycle frequency.
2. Select an appropriate prescaler/period.
3. Reset Timer1.
4. Clear its interrupt flag.
5. Enable the Timer1 interrupt if required for extending the hardware counter.
6. Start Timer1.

Timer1 must then run continuously.

Neither `millis()` nor `micros()` should reset or modify the timer.

---

# millis()

```c
uint32_t millis(void);
```

Return elapsed milliseconds since `timerTimebaseInit()`.

Intended usage:

```c
uint32_t previous = millis();

while (1) {
    if ((uint32_t)(millis() - previous) >= 1000) {
        previous = millis();

        // periodic operation
    }
}
```

Unsigned subtraction must be used for elapsed-time comparisons so normal `uint32_t` wraparound is handled correctly.

---

# micros()

```c
uint32_t micros(void);
```

Return elapsed microseconds since `timerTimebaseInit()`.

Use the software-extended Timer1 timebase together with the current Timer1 counter value so `micros()` has finer resolution than `millis()`.

Example:

```c
uint32_t start = micros();

/* operation */

uint32_t elapsed_us = micros() - start;
```

Natural `uint32_t` wraparound is acceptable.

---

# Timer2 — Global PWM Timebase

Timer2 is dedicated permanently to PWM.

All PWM-enabled pins share Timer2 and therefore share the same PWM frequency.

Provide:

```c
hal_status_t pwmFreq(uint32_t frequency_hz);
```

Example:

```c
pwmFreq(1000);
```

This configures the common PWM frequency to 1 kHz.

All active PWM outputs then operate at 1 kHz.

Individual PWM duty cycles remain controlled through the pin module:

```c
pinMode(PIN_6, PIN_PWM);
pwmWrite(PIN_6, 32768);
```

---

# pwmFreq()

Implement:

```c
hal_status_t pwmFreq(uint32_t frequency_hz);
```

The function should:

1. Use Timer2 only.
2. Obtain the instruction-cycle frequency from the clock configuration/HAL.
3. Calculate the required Timer2 prescaler.
4. Calculate the required Timer2 period.
5. Select the smallest valid prescaler that allows the period to fit in Timer2.
6. Stop Timer2 while reconfiguring it.
7. Set the prescaler.
8. Set `PR2`.
9. Reset `TMR2`.
10. Clear the Timer2 flag.
11. Start Timer2.
12. Return an error if the requested frequency cannot be represented.

Do not silently clamp unsupported frequencies.

Do not configure individual PWM pins or output-compare channels inside `timer.c`.

---

# Shared PWM Frequency

PWM frequency is global, not per-pin.

Do not implement:

```c
pwmSetFrequency(pin_t pin, uint32_t frequency_hz);
```

Use:

```c
pwmFreq(uint32_t frequency_hz);
```

Correct usage:

```c
pwmFreq(1000);

pinMode(PIN_6, PIN_PWM);
pinMode(PIN_7, PIN_PWM);

pwmWrite(PIN_6, 16384);
pwmWrite(PIN_7, 49152);
```

Both outputs use:

```text
frequency = 1000 Hz
```

but have independent duty cycles:

```text
PIN_6 -> approximately 25%
PIN_7 -> approximately 75%
```

A later call:

```c
pwmFreq(2000);
```

changes the common PWM frequency for all PWM outputs.

---

# Timer2 Period Access

The pin/PWM implementation needs the current Timer2 period to convert the normalized 16-bit PWM duty value into an output-compare value.

Provide:

```c
uint16_t pwmGetPeriod(void);
```

For example, `pin.c` may calculate:

```c
uint16_t period = pwmGetPeriod();
```

and scale:

```text
0     -> 0%
32768 -> approximately 50%
65535 -> 100%
```

to the Timer2 period.

Do not duplicate Timer2 frequency/prescaler calculations inside `pin.c`.

---

# Module Responsibilities

```text
timer.c
│
├── Timer1
│   ├── timerTimebaseInit()
│   ├── millis()
│   └── micros()
│
├── Timer2
│   ├── pwmFreq()
│   └── pwmGetPeriod()
│
└── Timer3
    └── UNUSED


pin.c
│
├── GPIO
├── ADC
└── PWM output
    ├── pinMode(PIN_x, PIN_PWM)
    └── pwmWrite()
```

`timer.c` owns the PWM clock/timebase.

`pin.c` owns the individual PWM output configuration and duty cycle.

`pin.c` must not directly configure:

```text
T2CON
TMR2
PR2
```

Timer3 must not be initialized, reserved, or modified.

---

# Public timer.h API

Keep the initial timer interface minimal:

```c
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#include "device.h"


/* Timer1 system timebase */

hal_status_t timerTimebaseInit(void);

uint32_t millis(void);

uint32_t micros(void);


/* Timer2 PWM timebase */

hal_status_t pwmFreq(uint32_t frequency_hz);

uint16_t pwmGetPeriod(void);


#endif
```

Do not expose:

```text
timer_t
timer_config_t
timerInit()
timerStart()
timerStop()
timerReset()
timerRead()
timerWrite()
timerSetPeriod()
timerSetFrequency()
timerSetPeriodUs()
timerSetPeriodMs()
timerClaim()
timerRelease()
```

These are unnecessary for the current HAL requirements.

---

# Timer3

Timer3 is intentionally unused.

Do not:

* initialize it
* stop/start it
* reserve it
* expose it through the API
* use it for PWM
* use it for the system timebase

Leave Timer3 completely untouched so it remains available for future functionality.

---

# File Structure

Add:

```text
include/
    timer.h

src/
    timer.c

examples/
    timer_example.c
```

---

# Example

`examples/timer_example.c` should demonstrate only the intended API:

```c
#include "timer.h"
#include "pin.h"

int main(void)
{
    timerTimebaseInit();

    pwmFreq(1000);

    pinMode(PIN_6, PIN_PWM);
    pwmWrite(PIN_6, 32768);

    uint32_t previous = millis();

    while (1) {
        if ((uint32_t)(millis() - previous) >= 1000) {
            previous = millis();

            // periodic operation
        }
    }

    return 0;
}
```

Keep the implementation PIC24F16KA101-specific and straightforward.

Do not add generic timer abstractions, software timers, schedulers, callbacks, RTOS functionality, dynamic allocation, or Timer3 functionality.

