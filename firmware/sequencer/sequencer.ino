#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Bounce2.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
const int8_t OLED_RESET  = -1; // Reset pin # (or -1 if sharing Arduino reset pin)

const int8_t PIN_ROT_IN1 = 9;
const int8_t PIN_ROT_IN2 = 10;

const int8_t PIN_ROT_BTN = 8;

Bounce apply_button = Bounce();

class DisplayManager
{
public:
  DisplayManager(int w=128, int h=64, int addr=0x3C) 
  : main_canvas_(w,FreeSans9pt7b.yAdvance), display_(w,h,&Wire,OLED_RESET), addr_(addr), inverted_(false)
  {
    main_canvas_.setFont(&FreeSans9pt7b);
    main_canvas_.setTextSize(1);
    main_canvas_.setTextColor(SSD1306_WHITE); 
  }

  bool begin() {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally 
    if(!display_.begin(SSD1306_SWITCHCAPVCC, addr_)) {
      return false;
    }
    display_.clearDisplay(); 
    display_.display();
    return true;
  }

  void invert()     { inverted_ = !inverted_; display_.invertDisplay(inverted_); }
  void clear_all()  { display_.clearDisplay(); }
  void show()       { display_.display(); }

  void update_position(int p) 
  {
    char msg[16];
    sprintf(msg, "%d", p);

    main_canvas_.fillScreen(SSD1306_BLACK);
    main_canvas_.setCursor(2, main_canvas_.height()-4);
    main_canvas_.print(msg);

    display_.drawBitmap(0,16,main_canvas_.getBuffer(),main_canvas_.width(),main_canvas_.height(),
      SSD1306_WHITE, SSD1306_BLACK);
  }

private:
  GFXcanvas1 main_canvas_;


  Adafruit_SSD1306 display_;
  int addr_;
  bool inverted_;
};


DisplayManager display;
// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);

// Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
RotaryEncoder encoder(PIN_ROT_IN2, PIN_ROT_IN1, RotaryEncoder::LatchMode::FOUR3);

// This interrupt routine will be called on any change of one of the input signals
void checkPosition()
{
  encoder.tick(); // just call tick() to check the state.
}


void setup() {
  Serial.begin(115200);
  pixels.begin();
  
  delay(1000);
  Serial.println("Starting sequencer");
  
  pixels.setPixelColor(0, pixels.Color(0, 96, 0));
  pixels.show();

  delay(1000);

  if (!display.begin()) {
    pixels.setPixelColor(0, pixels.Color(96, 0, 0));
    pixels.show();
    while(1); // wait forever
  } 
  pixels.clear();
  pixels.show();

  apply_button.attach(PIN_ROT_BTN, INPUT_PULLUP);
  apply_button.interval(50);

  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_ROT_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROT_IN2), checkPosition, CHANGE);

}

void loop() {
  static int pos = 0;

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
}
