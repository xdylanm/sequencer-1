#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <RotaryEncoder.h>
#include <SAMD_Utils.h>
#include "board_target.h"
#include "pin_definitions.h"
#include "adc_config.h"
#include "seqdisplay.h"
#include "controller.h"

#if defined(ADAFRUIT_QTPY_M0)
Adafruit_NeoPixel onboard_pixel(1, PIN_NEOPIXEL);
#elif defined(ADAFRUIT_ITSYBITSY_M4_EXPRESS) || defined(ADAFRUIT_ITSYBITSY_M0)
#include <Adafruit_DotStar.h>
Adafruit_DotStar onboard_pixel(1, 8, 6, DOTSTAR_BGR);
#endif


using namespace samd_utils;

// Display definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C 
SeqDisplay display(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS);

// create a status pixel strand with 8 pixels
Adafruit_NeoPixel_ZeroDMA status_pixels(8, PIN_STATUS_NEOPIXELS, NEO_GRBW + NEO_KHZ800);

// The controller for the roatary encoder
RotaryEncoder rot_encoder(PIN_ROT_IN1, PIN_ROT_IN2, RotaryEncoder::LatchMode::FOUR3);

// Settings for the ADC to define the base clock rate
adc_config_def adc_config;

// Settings for the DAC
#if defined(__SAMD21__) 
#define DAC_NUM_BITS 10
#elif defined(__SAMD51__)
#define DAC_NUM_BITS 12
#endif

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

#if defined(ADAFRUIT_QTPY_M0) 
int const addr_pins[] = {PAX_MUX_ADDR0, PAX_MUX_ADDR1, PAX_MUX_ADDR2};
#else
int const addr_pins[] = {PIN_MUX_ADDR0, PIN_MUX_ADDR1, PIN_MUX_ADDR2};
#endif

MachineState seq_state;   // State machine
Controller engine;        // Engine to control the timing for the sequencer

volatile int ch_ndx, next_ch_ndx;
volatile bool update_npxls, update_display;

// main timing for the engine comes from the conversion interrupts
#if defined(__SAMD21__) 
void ADC_Handler() 
{
  // process the ADC value & record the current step button state
  uint16_t const pot_val_raw = 0x0FFF & ADC->RESULT.reg;

  // should be OK to switch the MUX in between: settle time << sample time
  // we need to re-purpose the flash control pins on the QtPY to have enough pins
#if defined(ADAFRUIT_QTPY_M0) 
  int const step_key_raw = dio::iopin_read(PAX_MUXD_STEP_BUTTON);    // active high
  dio::iopin_write(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);
#else
  int const step_key_raw = digitalRead(PIN_MUXD_STEP_BUTTON);          // active high
  digitalWrite(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);
#endif

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

#elif defined(__SAMD51__)
void ADC0_1_Handler() 
{
  // process the ADC value & record the current step button state
  uint16_t const pot_val_raw = 0x0FFF & ADC0->RESULT.reg;

  // should be OK to switch the MUX in between: settle time << sample time
  // we need to re-purpose the flash control pins on the QtPY to have enough pins
  int const step_key_raw = digitalRead(PIN_MUXD_STEP_BUTTON);          // active high
  digitalWrite(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);

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

  ADC0->INTFLAG.bit.RESRDY = 1;
}
#endif

// ISR for the encoder pins -- call tick on any change
void check_position() 
{
  rot_encoder.tick();
}

void invalidate_npxls();
void invalidate_display();
void error_blink(int i);
void ok_blink();

void setup() {

  onboard_pixel.begin(); // Initialize pins for output
  onboard_pixel.setBrightness(80);
  onboard_pixel.show();  // Turn all LEDs off ASAP
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  pinMode(PIN_CV_DAC_OUT, OUTPUT);
  pinMode(PIN_RUN_STOP_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MODE_BUTTON, INPUT_PULLUP);
  pinMode(PIN_ROT_SW, INPUT_PULLUP);
  pinMode(PIN_GATE_OUT, OUTPUT);  
  //pinMode(PIN_ROT_IN1, INPUT);   // handled by rotary encoder: input w/ pullup
  //pinMode(PIN_ROT_IN2, INPUT);
  
#if defined(ADAFRUIT_QTPY_M0)
  dio::init_iopin_out(PAX_MUX_ADDR0);
  dio::init_iopin_out(PAX_MUX_ADDR1);
  dio::init_iopin_out(PAX_MUX_ADDR2);
  dio::init_iopin_in(PAX_MUXD_STEP_BUTTON);  // has an external 5.1k pull up

  dio::iopin_write(PAX_MUX_ADDR0,0);
  dio::iopin_write(PAX_MUX_ADDR1,0);
  dio::iopin_write(PAX_MUX_ADDR2,0);
#else
  pinMode(PIN_MUX_ADDR0, OUTPUT);
  pinMode(PIN_MUX_ADDR1, OUTPUT);
  pinMode(PIN_MUX_ADDR2, OUTPUT);
  pinMode(PIN_MUXD_STEP_BUTTON, INPUT_PULLUP);

  digitalWrite(PIN_MUX_ADDR0,0);
  digitalWrite(PIN_MUX_ADDR1,0);
  digitalWrite(PIN_MUX_ADDR2,0);
#endif

  if (!display.begin()) {
    Serial.println("Display initialization failed");
    error_blink(-1);
  }
  ok_blink();

  gclk::init(adc_config.gclk_id, adc_config.clk_div, adc_config.clk_divsel);

  adc::init_iopin_in(adc_config.adc_iopin, adc_config.adc_port);
  adc::init(adc_config.adc_inputctrl_muxpos_ain, adc_config.gclk_id, 
    adc_config.adc_prescaler, adc_config.adc_samplen, adc_config.adc_samplenum);

  analogWriteResolution(DAC_NUM_BITS);  // 10 bit or 12 bit DAC from board_target.h
  
  for (int i = 0; i < 13; ++i) {
    display.display_title("SEQUENCER-1", i);
    delay(50);
  }

  status_pixels.begin();
  for (int i = 0; i < 8; ++ i) {
    status_pixels.setPixelColor(i, status_pixels.Color(0, 48, 0));
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

  analogWrite(PIN_CV_DAC_OUT, 0);

  invalidate_npxls();
  invalidate_display();
  
  adc::start();

}

void loop() 
{
  if (update_npxls) {
    update_npxls = false;
    invalidate_npxls();
  }

  if (update_display) {
    update_display = false;
    invalidate_display();
  }
}


void invalidate_npxls() 
{
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

void invalidate_display()
{
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
    display.show();
}

void error_blink(int i) {

  if (i < 0) {
    while (1) {
        onboard_pixel.setPixelColor(0, status_pixels.Color(64, 0, 0));
        onboard_pixel.show();
        delay(200);
        onboard_pixel.clear();
        onboard_pixel.show();
        delay(400);
    }  
  } else {
    while (1) {
        status_pixels.setPixelColor(i, status_pixels.Color(64, 0, 0));
        status_pixels.show();
        delay(200);
        status_pixels.clear();
        status_pixels.show();
        delay(400);
    }
  }
}

void ok_blink()
{
  onboard_pixel.setPixelColor(0, onboard_pixel.Color(0, 64, 0));
  onboard_pixel.show();
  delay(200);
  onboard_pixel.clear();
  onboard_pixel.show();
  delay(300);
}
