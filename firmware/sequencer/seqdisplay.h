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

  void display_title(const char* t, int y_off = 12);
  void clear_all();

  void set_bpm(int bpm);
  void set_duty(int duty);
  void set_slide(int slide);

  void set_pattern(int pat);
  void set_quant(const char* q);
  void set_voct(int imin, int span);

  void set_mode_sel(const char* q);

  void select_top(int index);
  void activate_top(int index);

  void set_main_level_chart(int N, uint16_t const* cv, uint8_t const* active, uint8_t const* enabled);
  void set_main_level_bar(int step);

  void show();

private:
  GFXcanvas1 title_canvas_;
  GFXcanvas1 bpm_canvas_;
  GFXcanvas1 duty_canvas_;
  GFXcanvas1 slide_canvas_;
  GFXcanvas1 pattern_canvas_;
  GFXcanvas1 quant_canvas_;
  GFXcanvas1 mode_sel_canvas_;
  GFXcanvas1 voct_canvas_;
  GFXcanvas1 select_canvas_;
  GFXcanvas1 main_canvas_;
  
  Adafruit_SSD1306 display_;
  int addr_;

  int xoff_top_[8];

};


#endif