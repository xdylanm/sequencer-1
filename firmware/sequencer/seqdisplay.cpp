#include "seqdisplay.h"

#include "glyphs.h"

#include <Fonts/Org_01.h>

#define XOFF_BPM 1
#define XOFF_DUTY 20
#define XOFF_SLIDE 39
#define XOFF_PATTERN 58
#define XOFF_QUANT 77
#define XOFF_MODE_SEL 96
#define XOFF_VOCT 115

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
: title_canvas_(w, 16), bpm_canvas_(19, 16), duty_canvas_(19, 16), slide_canvas_(19,16),  
  pattern_canvas_(19,16), quant_canvas_(19,16), mode_sel_canvas_(19,16), voct_canvas_(12,16), 
  select_canvas_(w, 4), main_canvas_(w,88), display_(w, h, &Wire, OLED_RESET), addr_(addr)
{
  xoff_top_[0] = XOFF_BPM;
  xoff_top_[1] = XOFF_DUTY;
  xoff_top_[2] = XOFF_SLIDE;
  xoff_top_[3] = XOFF_PATTERN;
  xoff_top_[4] = XOFF_QUANT;
  xoff_top_[5] = XOFF_MODE_SEL;
  xoff_top_[6] = XOFF_VOCT;
  xoff_top_[7] = w;

  title_canvas_.setFont(&Org_01);
  title_canvas_.setTextSize(2);
  title_canvas_.setTextColor(SSD1306_WHITE);

  bpm_canvas_.setFont(&Org_01);
  bpm_canvas_.setTextSize(1);
  bpm_canvas_.setTextColor(SSD1306_WHITE);

  duty_canvas_.setFont(&Org_01);
  duty_canvas_.setTextSize(1);
  duty_canvas_.setTextColor(SSD1306_WHITE);

  slide_canvas_.setFont(&Org_01);
  slide_canvas_.setTextSize(1);
  slide_canvas_.setTextColor(SSD1306_WHITE);
  
  pattern_canvas_.setFont(&Org_01);
  pattern_canvas_.setTextSize(1);
  pattern_canvas_.setTextColor(SSD1306_WHITE);

  quant_canvas_.setFont(&Org_01);
  quant_canvas_.setTextSize(1);
  quant_canvas_.setTextColor(SSD1306_WHITE);

  mode_sel_canvas_.setFont(&Org_01);
  mode_sel_canvas_.setTextSize(1);
  mode_sel_canvas_.setTextColor(SSD1306_WHITE);

  voct_canvas_.setFont(&Org_01);
  voct_canvas_.setTextSize(1);
  voct_canvas_.setTextColor(SSD1306_WHITE);

}

bool SeqDisplay::begin()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display_.begin(SSD1306_SWITCHCAPVCC, addr_)) {
    return false;
  }
  display_.clearDisplay(); 
  display_.display(); 
}

void SeqDisplay::clear_all()
{
  display_.clearDisplay(); 
}

void SeqDisplay::display_title(const char *t, int const y_off /*=12*/) 
{
  title_canvas_.fillScreen(SSD1306_BLACK);
  title_canvas_.setCursor(3, y_off);
  title_canvas_.println(t);
  
  clear_all();
  display_.drawBitmap(0,0,title_canvas_.getBuffer(),title_canvas_.width(),title_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);
  display_.display();
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

  display_.drawBitmap(XOFF_BPM,0,bpm_canvas_.getBuffer(),bpm_canvas_.width(),bpm_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);
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

  display_.drawBitmap(XOFF_DUTY,0,duty_canvas_.getBuffer(),duty_canvas_.width(),duty_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::set_slide(int slide)
{
  if (slide < 0 || slide > 100) {
    return;
  }
  slide_canvas_.fillScreen(SSD1306_BLACK);

  if (slide == 0) {
    slide_canvas_.drawFastHLine(1, 8, 17, SSD1306_WHITE);
  } else if (slide == 100) {
    slide_canvas_.drawLine(1, 8, 17, 1, SSD1306_WHITE);
  } else {
    int const wlo = (17*(100-slide) + 50) / 100; // rounded int div 
    slide_canvas_.drawFastHLine(1, 8, wlo, SSD1306_WHITE);
    if (wlo < 17) {
      slide_canvas_.drawLine(1+wlo, 8, 17, 1, SSD1306_WHITE);
    } else {
      slide_canvas_.drawFastVLine(17, 1, 8, SSD1306_WHITE);
    }
  }

  char buf[4];
  int const xoff = int2str3_centered(&buf[0], slide_canvas_, slide, 17);
  slide_canvas_.setCursor(1 + xoff, 15);
  slide_canvas_.println(buf);

  display_.drawBitmap(XOFF_SLIDE,0,slide_canvas_.getBuffer(),slide_canvas_.width(),slide_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::set_pattern(int const pat) 
{
  if (pat < 0 || pat > 2) {
    return;
  }
  pattern_canvas_.fillScreen(SSD1306_BLACK);
  if (pat == 0) { // Loop
    pattern_canvas_.drawBitmap(0, 0, seq::pattern_loop, 19, 9, SSD1306_WHITE, SSD1306_BLACK);
    pattern_canvas_.setCursor(1, 15);
    pattern_canvas_.println("RPT");
  } else if (pat == 1) { // Bounce
    pattern_canvas_.drawBitmap(0, 0, seq::pattern_bounce, 19, 9, SSD1306_WHITE, SSD1306_BLACK);
    pattern_canvas_.setCursor(1, 15);
    pattern_canvas_.println("BNC");
  } else if (pat == 2) { // Bounce
    pattern_canvas_.drawBitmap(0, 0, seq::pattern_rand, 19, 9, SSD1306_WHITE, SSD1306_BLACK);
    pattern_canvas_.setCursor(1, 15);
    pattern_canvas_.println("RND");
  }

  display_.drawBitmap(XOFF_PATTERN,0,pattern_canvas_.getBuffer(),pattern_canvas_.width(),pattern_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::set_quant(const char* q) 
{
  quant_canvas_.fillScreen(SSD1306_BLACK);
  quant_canvas_.setCursor(1, 5);
  quant_canvas_.print("QNT");
  quant_canvas_.drawFastHLine(1, 8, 17, SSD1306_WHITE);
  quant_canvas_.setCursor(1, 15);
  quant_canvas_.println(q);

  display_.drawBitmap(XOFF_QUANT,0,quant_canvas_.getBuffer(),quant_canvas_.width(),quant_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::set_mode_sel(const char* q) 
{
  mode_sel_canvas_.fillScreen(SSD1306_BLACK);
  mode_sel_canvas_.setCursor(1, 5);
  mode_sel_canvas_.print("MOD");
  mode_sel_canvas_.drawFastHLine(1, 8, 17, SSD1306_WHITE);
  mode_sel_canvas_.setCursor(1, 15);
  mode_sel_canvas_.println(q);

  display_.drawBitmap(XOFF_MODE_SEL,0,mode_sel_canvas_.getBuffer(),mode_sel_canvas_.width(),mode_sel_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::set_voct(int imin, int span)
{
  if (imin < 0 || span < 1 || imin+span > 5) {
    return;
  }
  voct_canvas_.fillScreen(SSD1306_BLACK);
  for (int i = 0; i < 5; ++i) {
    voct_canvas_.drawFastHLine(1, 3*i + 2, 11, SSD1306_WHITE);
  }
  for (int i = 0; i < span; ++i) {
    int const y0 = 13 - 3*(i+imin);
    voct_canvas_.drawRect(2, y0, 9, 3, SSD1306_WHITE);
  }

  display_.drawBitmap(XOFF_VOCT,0,voct_canvas_.getBuffer(),voct_canvas_.width(),voct_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

 
}

void SeqDisplay::select_top(int const index)
{
  if (index < -1 || index > 6) {
    return;
  }
  select_canvas_.fillScreen(SSD1306_BLACK);
  if (index == -1) {
    return;
  }
  int const x0 = xoff_top_[index] + 1;
  int const w =  xoff_top_[index+1] - xoff_top_[index] - 2;
  select_canvas_.drawFastHLine(x0, 0, w, SSD1306_WHITE);
  select_canvas_.drawFastHLine(x0, 1, w, SSD1306_WHITE);

  display_.drawBitmap(0,16,select_canvas_.getBuffer(),select_canvas_.width(),select_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);

}

void SeqDisplay::activate_top(int const index)
{
  if (index < -1 || index > 6) {
    return;
  }
  select_canvas_.fillScreen(SSD1306_BLACK);
  if (index == -1) {
    return;
  }

  int const w =  xoff_top_[index+1] - xoff_top_[index];
  int const x0 = xoff_top_[index] + (w / 2);

  select_canvas_.drawFastHLine(xoff_top_[index]+1, 0, w-2, SSD1306_WHITE);
  select_canvas_.drawFastHLine(x0-1, 0, 3, SSD1306_BLACK);
  select_canvas_.drawPixel(x0, 0, SSD1306_WHITE);

  select_canvas_.drawFastHLine(xoff_top_[index]+1, 1, w-2, SSD1306_WHITE);
  select_canvas_.drawFastHLine(x0-2, 1, 5, SSD1306_BLACK);
  select_canvas_.drawFastHLine(x0-1, 1, 3, SSD1306_WHITE);
  
  select_canvas_.drawFastHLine(x0-2, 2, 5, SSD1306_WHITE);
  select_canvas_.drawFastHLine(x0-3, 3, 7, SSD1306_WHITE);

  display_.drawBitmap(0,16,select_canvas_.getBuffer(),select_canvas_.width(),select_canvas_.height(),
    SSD1306_WHITE,SSD1306_BLACK);
  
}

void SeqDisplay::show()
{
  display_.display();
}


void SeqDisplay::set_main_level_chart(int N, uint16_t const* cv, uint8_t const* active, uint8_t const* enabled)
{
  if (N != 8) {
    return;
  }
  int const y_off = main_canvas_.height()/2; 
  uint32_t const hy_max = main_canvas_.height()/2 - 4;
  int const y_bottom = y_off + hy_max + 1;  // y_off + 2 + hy_max - 1;

  main_canvas_.fillScreen(SSD1306_BLACK);
  for (int i = 0; i < N; ++i) {
    if (enabled[i]) {
      main_canvas_.drawRect(2 + i*16, y_off + 2, 12, hy_max-1, SSD1306_WHITE);
      uint32_t const hy = ((uint32_t)cv[i] * hy_max + 2048)/4096;   // 12-bit ADC
      int const y0 = y_bottom - hy + 2;
      if (active[i]) {
        main_canvas_.fillRect(2 + i*16, y0, 12, hy-1, SSD1306_WHITE);
      } else {
        main_canvas_.drawFastHLine(2 + i*16, y0, 12, SSD1306_WHITE);
      }
    } else {
      main_canvas_.drawFastHLine(2 + i*16, y_bottom, 12, SSD1306_WHITE);
    }
  }

  uint8_t* pstart = main_canvas_.getBuffer() + (y_off*main_canvas_.width())/8;
  display_.drawBitmap(0,20,pstart,main_canvas_.width(),main_canvas_.height()/2 - 1,
    SSD1306_WHITE,SSD1306_BLACK);
}

void SeqDisplay::set_main_level_bar(int const step)
{
  int const y_off = main_canvas_.height()-1; 
  main_canvas_.drawFastHLine(0, y_off, main_canvas_.width(), SSD1306_BLACK);
  if (step < 0) {
    return;
  }
  main_canvas_.drawFastHLine(2 + step*16, y_off, 12, SSD1306_WHITE);

  uint8_t* pstart = main_canvas_.getBuffer() + (y_off*main_canvas_.width())/8;
  display_.drawBitmap(0,63,pstart,main_canvas_.width(),1,
    SSD1306_WHITE,SSD1306_BLACK);

}



