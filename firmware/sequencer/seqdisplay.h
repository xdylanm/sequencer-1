#ifndef seqdisplay_h_
#define seqdisplay_h_

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class SeqDisplay
{
public:
  SeqDisplay(int w=128, int h=64, int addr=0x3C);

  bool begin();

  void set_bpm(int bpm);
  void set_duty(int duty);
  void set_slide(int slide);

  void set_pattern(int pat);
  void set_quant(const char* q);
  void set_voct(int imin, int span);

  void display_status();

  void select_top(int index);
  void activate_top(int index);

private:
  GFXcanvas1 bpm_canvas_;
  GFXcanvas1 duty_canvas_;
  GFXcanvas1 slide_canvas_;
  GFXcanvas1 pattern_canvas_;
  GFXcanvas1 quant_canvas_;
  GFXcanvas1 voct_canvas_;
  GFXcanvas1 select_canvas_;
  
  Adafruit_SSD1306 display_;
  int addr_;

  int xoff_top_[7];

};


#endif