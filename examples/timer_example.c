#include "timer.h"
#include "pin.h"

int main(void)
{
    timerTimebaseInit();

    pwmFreq(10000);

    pinMode(PIN_2, PIN_DOUT);
    pinMode(PIN_3, PIN_DOUT);


    uint32_t previous = millis();

    while (1) {
        if ((uint32_t)(millis() - previous) >= 1000u) {
            previous = millis();

            digitalToggle(PIN_2);
            digitalToggle(PIN_3);
        }
    }

    return 0;
}
