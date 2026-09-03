# PIC24F16KA101 Pin HAL — Codex Implementation Outline

## Target

Implement the initial Pin/GPIO/ADC/PWM HAL for:

* MCU: PIC24F16KA101
* Package: 20-pin SSOP
* Language: C
* Compiler headers: XC16 device headers
* Build system already exists

Do not perform:

* toolchain verification
* compiler installation
* programmer configuration
* hardware testing
* unit testing
* build-system restructuring
* CMake toolchain changes
* MPLAB configuration

I will handle building, programming, and hardware verification independently.

Codex should only implement the HAL source/header files and create one simple example source file showing intended API usage.

---

# 1. Pin Identifiers

Use physical package pin numbers as the public pin identifiers.

```c
typedef enum {
    PIN_1  = 1,
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
```

Do not expose PIC port names such as `RA0` or `RB4` through the public API.

Internally map package pin numbers to the appropriate PIC24 registers and register bits.

For now, assume the caller uses valid pins.

Do not implement runtime detection for attempts to configure VDD, VSS, MCLR, or other unavailable package pins.

---

# 2. Pin Modes

Implement:

```c
typedef enum {
    PIN_DIN,
    PIN_DIN_PULLUP,
    PIN_DOUT,
    PIN_DOUT_OPEN_DRAIN,
    PIN_AIN,
    PIN_PWM
} pin_mode_t;
```

Mode meanings:

* `PIN_DIN`

  * digital input
  * analog functionality disabled
  * TRIS configured as input

* `PIN_DIN_PULLUP`

  * digital input
  * analog functionality disabled
  * TRIS configured as input
  * internal pull-up enabled

* `PIN_DOUT`

  * digital push-pull output
  * analog functionality disabled
  * TRIS configured as output
  * open-drain disabled

* `PIN_DOUT_OPEN_DRAIN`

  * digital open-drain output
  * analog functionality disabled
  * TRIS configured as output
  * open-drain enabled

* `PIN_AIN`

  * analog input
  * TRIS configured as input
  * analog functionality enabled

* `PIN_PWM`

  * pin configured for PWM output
  * actual PWM operation handled by the PWM portion of this HAL

There is no general-purpose DAC on the PIC24F16KA101.

Do not implement `AOUT`.

---

# 3. Digital Pin State

Implement:

```c
typedef enum {
    LOW = 0,
    HIGH = 1
} pin_state_t;
```

---

# 4. Public Pin API

Implement:

```c
hal_status_t pinMode(pin_t pin, pin_mode_t mode);

hal_status_t digitalWrite(pin_t pin, pin_state_t state);

pin_state_t digitalRead(pin_t pin);

hal_status_t digitalToggle(pin_t pin);
```

## pinMode()

Configure all registers necessary for the requested mode.

This may include:

* TRIS
* ANS
* ODC
* CN pull-up registers

Only modify register bits belonging to the requested pin.

Do not modify unrelated pins.

## digitalWrite()

Write the requested digital state using the LAT register.

Do not perform output writes through PORT.

## digitalRead()

Read the physical state of the pin using PORT.

## digitalToggle()

Toggle only the requested LAT bit.

---

# 5. Analog Input API

Implement:

```c
uint16_t analogRead(pin_t pin);
```

`analogRead()` should:

* configure/select the appropriate ADC channel for the requested physical pin
* start an ADC conversion
* wait for the conversion to complete
* return the conversion result

Keep the first implementation blocking and simple.

Do not implement asynchronous ADC operation or interrupts.

---

# 6. PWM API

Implement:

```c
hal_status_t pwmWrite(pin_t pin, uint16_t value);

hal_status_t pwmSetFrequency(pin_t pin, uint32_t frequency_hz);
```

Use:

```text
0      = 0% duty cycle
65535  = 100% duty cycle
```

Values between these limits represent proportional duty cycle.

Example:

```c
pwmWrite(PIN_6, 0);
pwmWrite(PIN_6, 32768);
pwmWrite(PIN_6, 65535);
```

The PWM implementation should scale the 16-bit public duty-cycle value to the actual timer/output-compare resolution.

`pwmSetFrequency()` should configure the timer/output-compare hardware necessary to generate the requested frequency.

Keep the implementation blocking/configuration-based.

Do not implement PWM interrupts.

---

# 7. Pin Ownership

Maintain simple runtime ownership information so peripheral modules cannot silently take over pins already assigned to another subsystem.

Implement:

```c
typedef enum {
    PIN_OWNER_NONE,
    PIN_OWNER_GPIO,
    PIN_OWNER_ADC,
    PIN_OWNER_PWM,
    PIN_OWNER_UART,
    PIN_OWNER_I2C
} pin_owner_t;
```

Internal/public helper functions:

```c
hal_status_t pinClaim(pin_t pin, pin_owner_t owner);

void pinRelease(pin_t pin, pin_owner_t owner);
```

Rules:

* an unclaimed pin may be claimed
* a pin already owned by the same owner may continue to be used
* a pin owned by another subsystem must return `HAL_BUSY`
* releasing a pin sets its owner back to `PIN_OWNER_NONE`

Example:

```c
pinMode(PIN_6, PIN_DOUT);
```

claims the pin for GPIO.

A future UART implementation trying to use the same physical pin should receive `HAL_BUSY`.

Keep the ownership implementation simple using a static array indexed by `pin_t`.

---

# 8. Internal Pin Hardware Table

Create one internal lookup table that maps package pins to PIC24 hardware information.

Conceptually:

```c
typedef struct {
    volatile uint16_t *tris;
    volatile uint16_t *port;
    volatile uint16_t *lat;
    volatile uint16_t *ans;
    volatile uint16_t *odc;

    uint16_t mask;

    uint8_t adc_channel;
} pin_hw_t;
```

Example concept:

```c
static const pin_hw_t pin_hw[21] = {
    [PIN_2] = {
        .tris = &TRISA,
        .port = &PORTA,
        .lat  = &LATA,
        .ans  = ...,
        .odc  = ...,
        .mask = (1u << 0),
        .adc_channel = ...
    }
};
```

Index zero remains unused so the array index directly corresponds to the physical package pin number.

Use the actual PIC24F16KA101 20-pin SSOP pin mapping from the device definitions/datasheet when filling the table.

Do not guess peripheral/register mappings.

---

# 9. Runtime Pin State

Keep runtime state separate from constant hardware information.

Conceptually:

```c
typedef struct {
    pin_owner_t owner;
    pin_mode_t mode;
} pin_runtime_t;
```

Maintain:

```c
static pin_runtime_t pin_state[21];
```

The hardware table describes:

```text
what the physical pin is connected to
```

The runtime table describes:

```text
what the application is currently using the pin for
```

---

# 10. File Structure

Prefer a simple structure such as:

```text
include/
    pin.h

src/
    pin.c

examples/
    pin_example.c
```

Keep the implementation together for now.

Do not split GPIO, ADC, PWM, and pin ownership into separate modules unless there is a strong implementation reason to do so.

They can be separated later.

---

# 11. Example File

Create:

```text
examples/pin_example.c
```

The example exists only to demonstrate intended API usage.

It does not need automated verification.

Include examples of:

```c
pinMode(PIN_2, PIN_DOUT);
digitalWrite(PIN_2, HIGH);
digitalToggle(PIN_2);

pinMode(PIN_3, PIN_DIN);
pin_state_t input = digitalRead(PIN_3);

pinMode(PIN_4, PIN_DIN_PULLUP);

pinMode(PIN_5, PIN_AIN);
uint16_t adc_value = analogRead(PIN_5);

pinMode(PIN_6, PIN_PWM);
pwmSetFrequency(PIN_6, 1000);
pwmWrite(PIN_6, 32768);
```

Keep the example minimal.

Do not add delays, UART debugging, test frameworks, programmer commands, or hardware validation logic unless required simply for the example to compile.

---

# 12. Implementation Priorities

Implement in this order:

1. `pin_t`
2. internal physical pin mapping
3. runtime pin ownership
4. `pinMode()`
5. `digitalWrite()`
6. `digitalRead()`
7. `digitalToggle()`
8. `analogRead()`
9. `pwmSetFrequency()`
10. `pwmWrite()`
11. `pin_example.c`

Keep the code straightforward and PIC24F16KA101-specific.

Prefer direct readable register manipulation over unnecessary abstraction.

