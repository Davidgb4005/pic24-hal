#include "gpio.h"
#include "stdint.h"
#include <xc.h>
#include "timer.h"
int main(){
	timerTimebaseInit();
	uint16_t toggle_val = 0;
    pinMode(2,OUTPUT);
    pinMode(12,OUTPUT);
    pinMode(13,OUTPUT);
    pinMode(3,INPUT_PULLUP);
    pinMode(10,INPUT_PULLUP);
    pinMode(11,INPUT_PULLUP);
    pin_state_t input_pin = {
	    .pin_number = 3,
	    .edge_type = DEBOUNCE,
	    .debounce_delay = 1000,
	};
    AD1PCFG = 0xFFFF;
    uint32_t prev = millis();
    uint16_t btn_reg = 0;
    uint16_t toggle = 0;
    while(1){

    oneshotUpdate(&input_pin);
    if(0 && oneshotRead(&input_pin,READ)){
	toggle_val = toggle_val ^ 1;

    }
    digitalWrite(2,input_pin.output);
    btn_reg = digitalRead(10) |  digitalRead(11)<<1;
    uint16_t output_1 = 0;
    uint16_t output_2 = 0;
    switch(btn_reg){
        case 0:

        break;
        case 1:
            output_1 = 1;
        break;
        case 2:

            output_2 = 1;
        break;
        case 3:
            if (millis()-prev > 1000){
            prev = millis();
            toggle ^= 1;
        }

            output_1 = toggle;
            output_2 = toggle;
        break;
    }

        digitalWrite(12,output_1);
        digitalWrite(13,output_2);
    }
    return 0;
}
