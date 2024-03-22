#include "port_util.h"

// ADC input setting 
//  + SAMD21: AIN[1]/PA03/pin 4
//  + Adafruit QTPy: Pin 1/A1
void init_pin_for_ADC_in(int const iopin)
{
  PORT->Group[PORTA].PINCFG[iopin].reg=(uint8_t)(PORT_PINCFG_INEN);
  PORT->Group[PORTA].DIRCLR.reg = (1 << iopin);     // input PA03
  PORT->Group[PORTA].PINCFG[iopin].bit.PMUXEN = 1;  // enable peripheral mux to define behaviour
  PORT->Group[PORTA].PMUX[iopin >> 1].reg = PORT_PMUX_PMUXE_B | PORT_PMUX_PMUXO_B;  // type B for ADC
}

// digital out, initializes pin to 0
// example: SAMD21: PA07 is equivalent to Adafruit QTPy D7/A7/RX
void init_pin_for_D_out(int const iopin)
{
  PORT->Group[PORTA].PINCFG[iopin].reg=(uint8_t)(PORT_PINCFG_INEN);
  PORT->Group[PORTA].DIRSET.reg = (1 << iopin);     // output PA07
}

// digital in
void init_pin_for_D_in(int const iopin)
{
  PORT->Group[PORTA].PINCFG[iopin].reg=(uint8_t)(PORT_PINCFG_INEN);
  PORT->Group[PORTA].DIRCLR.reg = (1 << iopin);     // input at PA[iopin]
}

int iopin_digital_read(int iopin) 
{
  if ( (PORT->Group[PORTA].IN.reg & (1ul << iopin)) != 0) {
    return 1;
  }
  return 0;
}

void iopin_digital_write(int iopin, uint8_t val) 
{
  uint32_t const pinmask = (1ul << iopin);
  if (val == 0) {
    PORT->Group[PORTA].OUTCLR.reg = pinmask;
  } else {
    PORT->Group[PORTA].OUTSET.reg = pinmask;
  }
}
