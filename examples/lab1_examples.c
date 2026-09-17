#include "gpio.h"
#include "stdint.h"
#include <xc.h>
#include "timer.h"

#define PB1 9
#define PB2 10
#define PB3 11
#define LED1 12
#define LED2 13
#define LED3 14
#define DEBOUNCE_DELAY 100
#define BLINK_DELAY 500
int main()
{
	AD1PCFG = 0xFFFF;
	timerTimebaseInit();
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(PB1, INPUT_PULLUP);
	pinMode(PB2, INPUT_PULLUP);
	pinMode(PB3, INPUT_PULLUP);
	pin_state_t pin_state_array[] = {
		{
			.pin_number = PB1,
			.edge_type = DEBOUNCE,
			.debounce_delay = DEBOUNCE_DELAY,
		},
		{
			.pin_number = PB2,
			.edge_type = DEBOUNCE,
			.debounce_delay = DEBOUNCE_DELAY,
		},
		{
			.pin_number = PB3,
			.edge_type = DEBOUNCE,
			.debounce_delay = DEBOUNCE_DELAY,
		}};

	uint32_t prev = millis();
	uint16_t toggle = 0;
	uint16_t counter = 1;
	while (1)
	{
		if (millis()-prev > BLINK_DELAY){
			prev = millis();
			toggle ^= 1;
			counter++;
			if (counter == 9)
			{
				counter = 1;
			}
		}
		uint16_t btn_reg = 0;
		for (int i=0;i<sizeof(pin_state_array) / sizeof(pin_state_array[0]); i++){
			oneshotUpdate(&(pin_state_array[i]));
		}

		for (int i=0;i<sizeof(pin_state_array) / sizeof(pin_state_array[0]); i++){
			btn_reg |= oneshotRead(&(pin_state_array[i]),READ) << i;
		}

		
		//btn_reg = digitalRead(PB1) |  digitalRead(PB2) <<1 | digitalRead(PB3) << 2;

		uint16_t output_reg = 0;
		switch(btn_reg){
		case 0:
			break;

		case 1:
			output_reg |= toggle << 0;
			break;

		case 2:
			output_reg |= toggle << 1;
			break;

		case 3:
			output_reg |= (toggle ^ 1) << 1;
			output_reg |= toggle << 0;
			break;

		case 4:
			output_reg |= toggle << 2;
			break;

		case 5:
			output_reg |= (toggle ^ 1) << 2;
			output_reg |= toggle << 0;
			break;

		case 6:
			output_reg |= (toggle ^ 1) << 2;
			output_reg |= toggle << 1;
			break;

		case 7:
			output_reg = counter;
			break;
		default:
			break;
		}

		digitalWrite(LED1,(output_reg & 1) != 0);
		digitalWrite(LED2,(output_reg & 2) != 0);
		digitalWrite(LED3,(output_reg & 4) != 0);
	}
	return 0;
}
