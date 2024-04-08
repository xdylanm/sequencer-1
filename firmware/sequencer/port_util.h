#ifndef _port_util_h
#define _port_util_h

#include <Arduino.h>

void init_pin_for_ADC_in(int iopin);           // hardcoded for A1/PA03

void init_pin_for_D_out(int iopin);
void init_pin_for_D_in(int iopin);

int iopin_digital_read(int iopin);
void iopin_digital_write(int iopin, uint8_t val); 

#endif // _port_util_h
