#include "stdio.h"
#include "stdint.h"
#include "pin.h"
#include "scheduler.h"
#include "timer.h"
#include <xc.h>

uint16_t stack_a[128];
uint16_t stack_b[128];
task_handler_t taskA;
task_handler_t taskB;

void timer3_init(void)
{
    T3CONbits.TON = 0;

    T3CONbits.TCS   = 0;
    T3CONbits.TGATE = 0;
    T3CONbits.TCKPS = 0b11;   // 1:256

    TMR3 = 0;
    PR3 = 7812;               // ~1 second @ Fcy = 2 MHz

    IPC2bits.T3IP = 1;
    IFS0bits.T3IF = 0;
    IEC0bits.T3IE = 1;

    T3CONbits.TON = 1;
}


void function_1(void *my_parameters)
{
    uint32_t previous = millis();
    (void)my_parameters;
    while (1) {
            digitalWrite(PIN_2,LOW);
            digitalWrite(PIN_3,HIGH);
        }
    
}


void function_2(void *my_parameters)
{
    uint32_t previous = millis();
    (void)my_parameters;
    while (1) {
            digitalWrite(PIN_2,HIGH );
            digitalWrite(PIN_3,LOW);
        }
    
}

int main(){
    timerTimebaseInit();
    pinMode(PIN_2, PIN_DOUT);
    pinMode(PIN_3, PIN_DOUT);

    uint16_t parameters_a = 0;
    uint16_t parameters_b = 0;
    register_task(&taskA,function_1,&parameters_a,stack_a,128);
    register_task(&taskB,function_2,&parameters_b,stack_b,128);

    timer3_init();

    while (1) {

    }
    return 0;
}
