#include "lab1.h"
#include "stdint.h"
#include <xc.h>


int main(){
	uint16_t toggle_val = 0;
    pinMode(2,OUTPUT);
    uint16_t value = 1;
    pinMode(3,INPUT_PULLUP);
    pin_state_t input_pin = {
	    .pin_number = 3,
	    .edge_type = RISING_EDGE
	    .debounce_delay = 1000
	};
    AD1PCFG = 0xFFFF;
    while(1){
    oneshotUpdate(&input_pin);
    if(oneshotRead(&input_pin,CONSUME)){
	toggle_val = toggle_val ^ 1;

    }

    digitalWrite(2,toggle_val);
    }
    return 0;
}
