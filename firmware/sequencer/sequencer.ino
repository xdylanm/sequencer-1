#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
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
#define OLED_RESET -1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

Controller engine;

volatile int ch_ndx, next_ch_ndx;
volatile uint16_t ch0_val;

// main timing for the engine comes from the conversion interrupts
void ADC_Handler() 
{
  // process the ADC value & record the current step button state
  uint16_t const pot_val_raw = 0x0FFF & ADC->RESULT.reg;
  int const step_key_raw = iopin_digital_read(PAX_MUXD_STEP_BUTTON);   // active high
  
  if (ch_ndx == 0) {
    ch0_val = pot_val_raw > 0 ? pot_val_raw : 1;
  }

  // should be OK to switch the MUX in between: settle time << sample time
  iopin_digital_write(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);
  /*  
  // write the active CV & gate levels out
  analogWrite(PIN_CV_DAC_OUT, engine.cv());
  digitalWrite(PIN_GATE_OUT, engine.gate());

  int const run_stop_raw = digitalRead(PIN_RUN_STOP_BUTTON);
  int const mode_raw = digitalRead(PIN_MODE_BUTTON);
  engine.tick(ch_ndx, pot_val_raw, step_key_raw, run_stop_raw, mode_raw);
  */
  ch_ndx = next_ch_ndx;
  next_ch_ndx = (next_ch_ndx + 1) % 8;

  ADC->INTFLAG.bit.RESRDY = 1;  // write a bit to clear interrupt

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

int buttons_tmp[8];

void setup() {
  Serial.begin(115200);

  pinMode(PIN_CV_DAC_OUT, OUTPUT);
  pinMode(PIN_RUN_STOP_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MODE_BUTTON, INPUT_PULLUP);
  pinMode(PIN_ROT_SW, INPUT_PULLUP);
  pinMode(PIN_GATE_OUT, OUTPUT);  
  pinMode(PIN_ROT_IN1, INPUT);
  pinMode(PIN_ROT_IN2, INPUT);

  
  init_pin_for_D_out(PAX_MUX_ADDR0);
  init_pin_for_D_out(PAX_MUX_ADDR1);
  init_pin_for_D_out(PAX_MUX_ADDR2);
  init_pin_for_D_in(PAX_MUXD_STEP_BUTTON);  // has an external 5.1k pull up
  
  init_GCLK(5, adc_config.clk_div, adc_config.clk_divsel);
  init_pin_for_ADC_in(PAX_MUXD_STEP_POT);
  init_ADC(ADC_INPUTCTRL_MUXPOS_AIN, GCLK_CLKCTRL_GEN_GCLK5, adc_config.adc_prescaler, adc_config.adc_samplen, adc_config.adc_samplenum);
  
  analogWriteResolution(10);  // 10 bit DAC

  status_pixels.begin();
  
  delay(1000);
  Serial.println("Starting sequencer");
  
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
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Display initialization failed");
    error_blink(0);
  }
    // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.display();

  ch_ndx = 0;
  next_ch_ndx = 1;
  for (int i = 0; i < 8; ++i) {
    buttons_tmp[i] = 0;
  }

  iopin_digital_write(PAX_MUX_ADDR0,0);
  iopin_digital_write(PAX_MUX_ADDR1,0);
  iopin_digital_write(PAX_MUX_ADDR2,0);


  start_ADC();
}

void loop() 
{

  if (ch0_val > 0) {
    //Serial.println(ch0_val);
    uint16_t lvl = ((ch0_val >> 9) & 0x0007) + 1; // 12 bits >> 9 bits = 3 bits ==> 0-7
    ch0_val = 0;

    for (int i = 0; i < lvl; ++i) {
      status_pixels.setPixelColor(i, status_pixels.Color(0, 96, 0));
    }
    for (int i = lvl; i < 8; ++i) {
      status_pixels.setPixelColor(i, status_pixels.Color(0, 0, 0));
    }
    status_pixels.show();
  }


  /*
  int const step_key_raw = iopin_digital_read(PAX_MUXD_STEP_BUTTON);   // active low
  buttons_tmp[ch_ndx] = step_key_raw == 0 ? 1 : 0;

  // should be OK to switch the MUX in between: settle time << sample time
  iopin_digital_write(addr_pins[mux_addr_gray_table[2*next_ch_ndx]], mux_addr_gray_table[2*next_ch_ndx + 1]);

  for (int i = 0; i < 8; ++i) {
    status_pixels.setPixelColor(i, status_pixels.Color(0, 96*buttons_tmp[i], 0));
  }
  status_pixels.show();

  ch_ndx = next_ch_ndx;
  next_ch_ndx = (next_ch_ndx + 1) % 8;
  delay(5);
*/




  
/*
  // put your main code here, to run repeatedly:
  int new_pos = encoder.getPosition();
  if (pos != new_pos) {
    pos = new_pos;
    display.update_position(new_pos);
    display.show();

    pixels.setPixelColor(0, pixels.Color(0, 0, 32));
    pixels.show();
    delay(100);
    pixels.clear();
    pixels.show();
  }

  apply_button.update();
  if (apply_button.fell()) {
    display.invert();
  }
  */
}
