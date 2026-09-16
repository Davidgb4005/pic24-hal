#include "lab1.h"
#include "stdint.h"
#include <xc.h>
#include "timer.h"
int main(){
	timerTimebaseInit();
	uint16_t toggle_val = 0;
    pinMode(2,OUTPUT);
    pinMode(3,INPUT_PULLUP);
    pin_state_t input_pin = {
	    .pin_number = 3,
	    .edge_type = DEBOUNCE,
	    .debounce_delay = 1000,
	};
    AD1PCFG = 0xFFFF;
    uint32_t prev = millis();
    while(1){

    oneshotUpdate(&input_pin);
    if(0 && oneshotRead(&input_pin,READ)){
	toggle_val = toggle_val ^ 1;

    }
    if ((millis()-prev > 1000) && 0){
	//toggle_val^= 1;
	prev = millis();}


    digitalWrite(2,input_pin.output);
    }
    return 0;
}
