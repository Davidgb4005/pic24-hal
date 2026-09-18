#include "pin.h"

int state = 0;
void main(void)
{
while(0){	
    pinMode(PIN_2, PIN_DOUT);

    pinMode(PIN_3, PIN_DIN_PULLUP);
    pin_state_t input = digitalRead(PIN_3);
	if (input && !state){
    digitalToggle(PIN_2);
    state = 1;
	}
	if (!input && state){
		state = 0;
	}


}
while(1){


}
}
