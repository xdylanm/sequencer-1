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

  void set_duty(int duty);
  void set_bpm(int bpm);

  void display_status();

private:
  GFXcanvas1 bpm_canvas_;
  GFXcanvas1 duty_canvas_;
  Adafruit_SSD1306 display_;
  int addr_;


};


#endif