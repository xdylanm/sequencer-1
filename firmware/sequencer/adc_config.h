#pragma once
#include "board_target.h"

/*!
  \file adc_config.h

  \brief Board- and microcontroller-specific configuration for ADC 

  Different IO pins are defined depending on the selected board. Additionally,
  different register configurations are applied depending on the microcontroller
  architecture (SAMD21 or SAMD51). 

  In all cases, these settings determine the main clock rate for the
  sequencer (i.e. the DAC update rate for CV and sample rates for the 
  mux'd in controls). 

  Debugging/clock validation is also possible: in the setup, add a statement to 
  initialize the GCLK IO pin for output and pass the debug IO flag (true) to the
  `gclk::init` function (see SAMD_Utils.h)

    gclk::init_iopin_out(adc_config.gclk_iopin, adc_config.gclk_port);  // PA17, GCLK[3] -- D13 on the ItsyBitsy
    gclk::init(adc_config.gclk_id, adc_config.clk_div, adc_config.clk_divsel, true);
    
  \note The pins available for GCLK multiplexing are significantly restricted. 
  Check the SAMD datasheet IO multiplexing table to verify. Using the debug pin 
  may impact other functionality.  
*/

struct adc_config_def 
{
  // conversion time 
  // T_conv = (7.0 + 0.5*N) * 2^(P+2) * D * Navg [CPU clock ticks]
  //   + N: samplen (0-63)
  //   + P: prescaler (0-7)
  //   + D: clock divider (0-255, treat as divsel=direct)

  // the physical pins are different
#if defined(ADAFRUIT_QTPY_M0)
  static const uint8_t gclk_id = 5;
  static const uint8_t gclk_port = 0;       // pins only used for debug out
  static const uint8_t gclk_iopin = 11;

  static const uint8_t adc_port = 0;        // 0=PORTA, 1=PORTB
  static const uint8_t adc_iopin = 2;       // PxN, N = iopin, x = port 
  static const uint8_t adc_inputctrl_muxpos_ain = 0;    // AIN[x]
#elif defined(ADAFRUIT_ITSYBITSY_M0) 
  static const uint8_t gclk_id = 3;
  static const uint8_t gclk_port = 0;
  static const uint8_t gclk_iopin = 17;

  static const uint8_t adc_port = 0;        // 0=PORTA, 1=PORTB
  static const uint8_t adc_iopin = 5;       // PxN, N = iopin, x = port 
  static const uint8_t adc_inputctrl_muxpos_ain = 5;    // AIN[x]
#elif defined(ADAFRUIT_ITSYBITSY_M4_EXPRESS) 
  static const uint8_t gclk_id = 3;
  static const uint8_t gclk_port = 0;
  static const uint8_t gclk_iopin = 17;

  static const uint8_t adc_port = 0;        // 0=PORTA, 1=PORTB
  static const uint8_t adc_iopin = 4;       // PxN, N = iopin, x = port 
  static const uint8_t adc_inputctrl_muxpos_ain = 4;    // AIN[x]
#else
  #error Unsupported board for ADC configuration
#endif
  
// discriminate between SAMD21 and SAMD51 boards
#if defined(__SAMD21__) 
  static const uint8_t clk_div = 75;
  static const int clk_divsel = samd_utils::GCLK_DIVSEL_DIRECT;
  
  static const uint8_t adc_prescaler = 0;   // 2^(0+2) = 4
  static const uint8_t adc_samplen = 6;     // 1[gain] + 12[bits]/2 + 6[samplen]/2 = 10
  static const uint8_t adc_samplenum = 3;   // 8x avg

  // 48000kHz / 75(clk div) / 4(prescaler) = 160kHz / 8(avg) / (3 samplen + 12/2 bits + 1 gain) = 2kSPS
#elif defined(__SAMD51__)
  static const uint8_t clk_div = 75;
  static const int clk_divsel = samd_utils::GCLK_DIVSEL_DIRECT;
  
  static const uint8_t adc_prescaler = 0;   // 2^(0+1) = 2
  static const uint8_t adc_samplen = 7;     // 1 + 7 = 8
  static const uint8_t adc_samplenum = 3;   // 8x avg

  // 48000kHz / 75(clk div) / 2 (prescaler) = 320kHz / 8(avg) / (8 samplelen + 12 bits) = 2kSPS
#endif
}; 