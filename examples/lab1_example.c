#include "lab1.h"
#include "stdint.h"
#include <xc.h>


int main(){
    pinMode(2,OUTPUT);
    uint16_t value = 1;
    pinMode(3,INPUT_PULLUP);
    AD1PCFG = 0xFFFF;
    while(1){
    
    digitalWrite(2,digitalRead(3));

    }
    return 0;
}