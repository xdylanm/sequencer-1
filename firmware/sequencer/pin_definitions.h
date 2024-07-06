#pragma once

/*!
  \file pin_definitions.h

  \brief Labels for physical pins

  To support different boards (QT Py & Itsy Bitsy variants), different 
  pin assignments are required. Not all capabilities are available on
  all boards. 

  Defines starting with `PIN_` refer to "Arduino" pin numbers (board-dependent).

  Defines starting with `PAX_` refer to the IO pin number from the IO
  multiplexing table in the SAMD datasheets (when the pin is not broken out to
  a header on the board or when required to configure the peripheral).
  
*/

// Pin definitions
//  + PIN_ prefix for Arduino numbering
//  + PAX_ prefix for SAMD21 IO pin numbering (e.g. SAMD PA02 = Arduino A0/D0 on the QtPY) 

////////////////////// QT Py /////////////////////////////////////
#if defined(ADAFRUIT_QTPY_M0)

#define PIN_CV_DAC_OUT 0
// pin 1 is POT_MUX (PA03)
#define PIN_RUN_STOP_BUTTON 2
#define PIN_MODE_BUTTON 3
// pin 4 & 5 (SDA & SCL) for the display I2C
#define PIN_ROT_SW 6
#define PIN_GATE_OUT 7
#define PIN_ROT_IN1 8
#define PIN_ROT_IN2 9
#define PIN_STATUS_NEOPIXELS 10 // must be MOSI for DMA 

// pin definitions for ADC found in adc_config

// QTPy needs to use pins from Flash interface
#define PAX_MUXD_STEP_BUTTON 8  // FLASH_CS with 5k1 pullup
#define PAX_MUX_ADDR0 19
#define PAX_MUX_ADDR1 22
#define PAX_MUX_ADDR2 23

//////////////// ITSY BITSY M0 (SAMD21) and M4 (SAMD51) ///////////////////////
#elif defined(ADAFRUIT_ITSYBITSY_M4_EXPRESS) || defined(ADAFRUIT_ITSYBITSY_M0) 

#define PIN_CV_DAC_OUT 0  // A0/D14
// pin D15-D17 are not used
// pin D18 is POT_MUX (PA04)
#define PIN_SYNC_IN 19
#define PIN_SYNC_OUT 24

#define PIN_MODE_BUTTON 25
#define PIN_RUN_STOP_BUTTON 23

#define PIN_GATE_OUT 2

// pin D3 and D4 are not used

#define PIN_MUX_ADDR0 0
#define PIN_MUX_ADDR1 1
#define PIN_MUX_ADDR2 7
// pin 21 & 22 (SDA & SCL) for the display I2C

#define PIN_STATUS_NEOPIXELS 5 // DMA capable with a built-in 5V level shifter

#define PIN_MUXD_STEP_BUTTON 9

#define PIN_ROT_IN1 10
#define PIN_ROT_IN2 11

#define PIN_ROT_SW 12

// D13 is not used

// pin definitions for ADC found in adc_config, different for M0 & M4

////////////////////// Undefined /////////////////////////////////
#else
#error Unsupported board, pins not defined
#endif
