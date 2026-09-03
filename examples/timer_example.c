#include "timer.h"
#include "pin.h"

int main(void)
{
    timerTimebaseInit();

    pwmFreq(10000);

    pinMode(PIN_2, PIN_DOUT);

    pinMode(PIN_14, PIN_PWM);
    pwmWrite(PIN_14, 32768);

    uint32_t previous = millis();

    while (1) {
        if ((uint32_t)(millis() - previous) >= 1000u) {
            previous = millis();

            digitalToggle(PIN_2);
        }
    }

    return 0;
}
