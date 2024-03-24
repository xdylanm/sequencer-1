#include "seqdisplay.h"

#include "glyphs.h"

#include <Fonts/Org_01.h>

#define XOFF_BPM 1
#define XOFF_DUTY 20
#define XOFF_SLIDE 39

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

namespace {

int int2str3_centered(char* buf, GFXcanvas1& canvas, int num, int w_max)
{
  sprintf(buf,"%d",num);
  int16_t  x1, y1;
  uint16_t w, h;
  canvas.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  return (w < w_max ? w_max - w : 0) / 2;
}

}


SeqDisplay::SeqDisplay(int w /*=128*/, int h /*=64 */, int addr /*=0x3C*/)
: bpm_canvas_(19, 16), duty_canvas_(19, 16), display_(w, h, &Wire, OLED_RESET), addr_(addr)
{
  bpm_canvas_.setFont(&Org_01);
  bpm_canvas_.setTextSize(1);
  bpm_canvas_.setTextColor(SSD1306_WHITE);

  duty_canvas_.setFont(&Org_01);
  duty_canvas_.setTextSize(1);
  duty_canvas_.setTextColor(SSD1306_WHITE);
}

bool SeqDisplay::begin()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display_.begin(SSD1306_SWITCHCAPVCC, addr_)) {
    return false;
  }
  display_.clearDisplay(); 
}

void SeqDisplay::set_bpm(int bpm)
{
  if (bpm < 0 || bpm > 255) {
    return;
  }
  
  bpm_canvas_.fillScreen(SSD1306_BLACK);
  bpm_canvas_.drawBitmap(0, 0, seq::bpm_heart, 19, 10, SSD1306_WHITE, SSD1306_BLACK);

  char buf[4];
  int const xoff = int2str3_centered(&buf[0], bpm_canvas_, bpm, 17);
  bpm_canvas_.setCursor(1 + xoff, 15);
  bpm_canvas_.println(buf);
}

void SeqDisplay::set_duty(int duty)
{
  if (duty < 0 || duty > 100) {
    return;
  }
  duty_canvas_.fillScreen(SSD1306_BLACK);

  if (duty == 0) {
    duty_canvas_.drawFastHLine(1, 8, 17, SSD1306_WHITE);
  } else if (duty == 100) {
    duty_canvas_.drawFastHLine(1, 1, 17, SSD1306_WHITE);
  } else {
    duty_canvas_.drawFastVLine(1, 1, 8, SSD1306_WHITE);
    int const whi = (17*duty + 50) / 100; // rounded int div 
    duty_canvas_.drawFastHLine(1, 1, whi, SSD1306_WHITE);
    duty_canvas_.drawFastVLine(1+whi, 1, 8, SSD1306_WHITE);
    if (whi < 17) {
      duty_canvas_.drawFastHLine(1+whi, 8, 17-whi, SSD1306_WHITE);
    }
  }

  char buf[4];
  int const xoff = int2str3_centered(&buf[0], duty_canvas_, duty, 17);
  duty_canvas_.setCursor(1 + xoff, 15);
  duty_canvas_.println(buf);
}

void SeqDisplay::display_status()
{
  display_.drawBitmap(XOFF_BPM,0,bpm_canvas_.getBuffer(),bpm_canvas_.width(),bpm_canvas_.height(),
      SSD1306_WHITE,SSD1306_BLACK);
  display_.drawBitmap(XOFF_DUTY,0,duty_canvas_.getBuffer(),duty_canvas_.width(),duty_canvas_.height(),
      SSD1306_WHITE,SSD1306_BLACK);
  display_.display();
}

