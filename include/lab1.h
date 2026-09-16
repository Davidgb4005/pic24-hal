

#include <stdint.h>


typedef enum{
    UNINIT,
    INPUT,
    INPUT_PULLUP,
    INPUT_PULLDOWN,
    OUTPUT,
}mode_t;

typedef enum{
	DEBOUNCE,
	RISING_EDGE,
	FALLING_EDGE,
	BOTH_EDGE,
}edge_type_t;
typedef enum{
	CONSUME,
	READ,
	RESET,
}read_type_t;

typedef struct{
	uint16_t pin_number;	
	uint16_t cur_state;
	uint16_t prev_state;
	uint16_t debounce_state;
	uint32_t debounce_time; //prev time holder variable
	uint16_t debounce_delay; //offset for db time
	uint16_t debounce_output;
	uint16_t output;
	edge_type_t edge_type;
}pin_state_t;
uint16_t pinMode(uint16_t pin,mode_t mode);
uint16_t digitalRead(uint16_t pin);
void digitalWrite(uint16_t pin,uint16_t value);
void oneshotUpdate(pin_state_t * pin_state);
uint16_t oneshotRead(pin_state_t * pin_state,read_type_t read_type);
