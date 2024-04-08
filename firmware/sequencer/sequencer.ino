#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <RotaryEncoder.h>
#include "seqdisplay.h"
#include "controller.h"
#include "port_util.h"
#include "gclk_util.h"
#include "adc_util.h"

// Pin definitions
//  + PIN_ prefix for Arduino numbering
//  + PAX_ prefix for SAMD21 IO pin numbering (e.g. SAMD PA02 = Arduino A0/D0 on the QtPY) 
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

#define ADC_INPUTCTRL_MUXPOS_AIN 1
#define PAX_MUXD_STEP_POT 3     // AIN[1]
#define PAX_MUXD_STEP_BUTTON 8  // FLASH_CS with 5k1 pulldown
#define PAX_MUX_ADDR0 19
#define PAX_MUX_ADDR1 22
#define PAX_MUX_ADDR2 23

// Display definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C 
SeqDisplay display(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS);

// create a status pixel strand with 8 pixels
Adafruit_NeoPixel_ZeroDMA status_pixels(8, PIN_STATUS_NEOPIXELS, NEO_GRBW + NEO_KHZ800);

struct adc_config_def 
{
  // conversion time 
  // T_conv = (7.0 + 0.5*N) * 2^(P+2) * D * Navg [CPU clock ticks]
  //   + N: samplen (0-63)
  //   + P: prescaler (0-7)
  //   + D: clock divider (0-255, treat as divsel=direct)

  static const uint8_t clk_div = 80;
  static const DIVSEL_T clk_divsel = GCLK_DIVSEL_DIRECT;
  static const uint8_t adc_prescaler = 0;   // 2^(0+2) = 4
  static const uint8_t adc_samplen = 5;     // 7 + 0.5*(1+5) = 10
  static const uint8_t adc_samplenum = 3;   // 8x avg
} adc_config; 

// decimal equivalent, bit to change, new bit state
uint8_t const mux_addr_gray_table[] = {
  2, 0,  // 100 -> 000
  0, 1,  // 000 -> 001
  1, 1,  // 001 -> 011
  0, 0,  // 011 -> 010
  2, 1,  // 010 -> 110
  0, 1,  // 110 -> 111
  1, 0,  // 111 -> 101
  0, 0   // 101 -> 100
};
int const addr_pins[] = {PAX_MUX_ADDR0, PAX_MUX_ADDR1, PAX_MUX_ADDR2};

MachineState seq_state;
Controller engine;
RotaryEncoder rot_encoder(PIN_ROT_IN2, PIN_ROT_IN1, RotaryEncoder::LatchMode::FOUR3);

volatile int ch_ndx, next_ch_ndx;
volatile bool update_npxls, update_display;

// main timing for the engine comes from the conversion interrupts
void ADC_Handler() 
{
  // process the ADC value & record the current step button state
  uint16_t const pot_val_raw = 0x0FFF & ADC->RESULT.reg;
  int const step_key_raw = iopin_digital_read(PAX_MUXD_STEP_BUTTON);   // active high
  
  // should be OK to switch the MUX in between: settle time << sample time
  iopin_digital_write(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);

  // write the active CV & gate levels out
  analogWrite(PIN_CV_DAC_OUT, engine.cv(seq_state));
  digitalWrite(PIN_GATE_OUT, engine.gate(seq_state));

  int const run_stop_raw = digitalRead(PIN_RUN_STOP_BUTTON);
  int const mode_raw = digitalRead(PIN_MODE_BUTTON);
  int const rot_sw_raw = digitalRead(PIN_ROT_SW);
  
  update_npxls |= engine.tick(seq_state);

  seq_state.push(ch_ndx, pot_val_raw, step_key_raw);
  uint8_t const res = seq_state.process_key_events(run_stop_raw, mode_raw, rot_sw_raw, rot_encoder.getPosition());
  update_npxls |= (bool)(res & INVALIDATE_NPXLS);
  update_display |= (bool)(res & INVALIDATE_OLED);

  engine.update_parameters(seq_state);



  ch_ndx = next_ch_ndx;
  next_ch_ndx = (next_ch_ndx + 1) % MAX_NUM_STEPS;

  ADC->INTFLAG.bit.RESRDY = 1;  // write a bit to clear interrupt

}

void check_position() 
{
  rot_encoder.tick();
}

void error_blink(int i) {
  while (1) {
      status_pixels.setPixelColor(i, status_pixels.Color(64, 0, 0));
      status_pixels.show();
      delay(200);
      status_pixels.clear();
      status_pixels.show();
      delay(400);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_CV_DAC_OUT, OUTPUT);
  pinMode(PIN_RUN_STOP_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MODE_BUTTON, INPUT_PULLUP);
  pinMode(PIN_ROT_SW, INPUT_PULLUP);
  pinMode(PIN_GATE_OUT, OUTPUT);  
  //pinMode(PIN_ROT_IN1, INPUT);   // handled by rotary encoder: input w/ pullup
  //pinMode(PIN_ROT_IN2, INPUT);
  
  init_pin_for_D_out(PAX_MUX_ADDR0);
  init_pin_for_D_out(PAX_MUX_ADDR1);
  init_pin_for_D_out(PAX_MUX_ADDR2);
  init_pin_for_D_in(PAX_MUXD_STEP_BUTTON);  // has an external 5.1k pull up
  
  init_GCLK(5, adc_config.clk_div, adc_config.clk_divsel);
  init_pin_for_ADC_in(PAX_MUXD_STEP_POT);
  init_ADC(ADC_INPUTCTRL_MUXPOS_AIN, GCLK_CLKCTRL_GEN_GCLK5, adc_config.adc_prescaler, adc_config.adc_samplen, adc_config.adc_samplenum);
  
  analogWriteResolution(10);  // 10 bit DAC
  
  //Serial.println("Starting sequencer");
  
  if (!display.begin()) {
    Serial.println("Display initialization failed");
    error_blink(0);
  }
  delay(500);
  for (int i = 0; i < 13; ++i) {
    display.display_title("SEQUENCER-1", i);
    delay(50);
  }

  status_pixels.begin();
  for (int i = 0; i < 8; ++ i) {
    status_pixels.setPixelColor(i, status_pixels.Color(0, 96, 0));
    if (i > 0) {
      status_pixels.setPixelColor(i-1, status_pixels.Color(0, 0, 0));
    }
    status_pixels.show();
    delay(500);
  }
  status_pixels.clear();
  status_pixels.show();
  
  delay(1000);
  for (int i = 13; i < 32; ++i) {
    display.display_title("SEQUENCER-1", i);
    delay(50);
  }
  display.clear_all();

  seq_state.set_rotary_position(rot_encoder.getPosition());
    // register interrupt routine for rotary encoder
  attachInterrupt(digitalPinToInterrupt(PIN_ROT_IN1), check_position, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROT_IN2), check_position, CHANGE);
  
  ch_ndx = 0;
  next_ch_ndx = 1;

  seq_state.tick_freq(2000);
  engine.update_parameters(seq_state);
    
  iopin_digital_write(PAX_MUX_ADDR0,0);
  iopin_digital_write(PAX_MUX_ADDR1,0);
  iopin_digital_write(PAX_MUX_ADDR2,0);

  update_npxls = true;
  update_display = true;  // one shot

  start_ADC();
}

void loop() 
{

  if (update_npxls) {
    update_npxls = false;
    for (int i = 0; i < MAX_NUM_STEPS; ++i) {
      status_pixels.setPixelColor(i, seq_state.pixel_color(i));
    }
    status_pixels.show();

    display.set_main_level_chart(8, seq_state.cv_buf(), seq_state.step_active_buf(), seq_state.step_enable_buf());
    if (seq_state.running()) {
      display.set_main_level_bar(seq_state.current_step());
    }
    display.show();
  }

  if (update_display) {
    
    display.set_bpm(seq_state.bpm());
    display.set_duty(seq_state.duty_pct());
    display.set_slide(seq_state.slide_pct());
    
    display.set_pattern((int)seq_state.pattern());
    
    switch(seq_state.quant()) {
      case MachineState::Quantization::NONE:
        display.set_quant("LIN");
        break;
      case MachineState::Quantization::CHROMATIC:
        display.set_quant("CRO");
        break;
      case MachineState::Quantization::MAJOR:
        display.set_quant("MAJ");
        break;
      case MachineState::Quantization::MINOR:
        display.set_quant("MIN");
        break;
    }
    
    switch(seq_state.step_button_mode()) {
      case MachineState::StepButtonMode::STEP_ACTIVE:
        display.set_mode_sel("ACT");
        break;
      case MachineState::StepButtonMode::STEP_ENABLE:
        display.set_mode_sel("EN");
        break;
      default:
        break;  

    }

    switch(seq_state.voct_range()) {
      case MachineState::OutputRange::VOCT_5:
        display.set_voct(0, 5);
        break;
      case MachineState::OutputRange::VOCT_2:
        display.set_voct(2 + seq_state.octave_shift(), 2);
        break;
      case MachineState::OutputRange::VOCT_1:
        display.set_voct(2 + seq_state.octave_shift(), 1);
        break;
    }

    if (seq_state.menu_state().editing) {
      display.activate_top(seq_state.menu_state().active_menu_item);
    } else {
      display.select_top(seq_state.menu_state().active_menu_item);
    }

    update_display = false;

    display.show();

  }

}
