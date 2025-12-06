#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

#define NPXL_PIN 7
#define NPXL_COUNT 4
#define XLED_PIN 6
Adafruit_NeoPixel strip(NPXL_COUNT, NPXL_PIN, NEO_GRB + NEO_KHZ800);

#define BUTTON_COUNT 4
Bounce button[] = {Bounce(), Bounce(), Bounce(), Bounce()};

void setup() 
{
    strip.begin();
    strip.clear();
    strip.show();

    pinMode(XLED_PIN, OUTPUT);

    for (int i = 0; i < BUTTON_COUNT; ++i) {    
        pinMode(i, INPUT_PULLUP);
        button[i].attach(i);
        button[i].interval(5);
    }

    for (int i = 0; i < BUTTON_COUNT; ++i) {
        strip.clear();
        strip.setPixelColor(i,32,0,32);
        strip.show();
        digitalWrite(XLED_PIN,HIGH);
        delay(500);
        strip.clear();
        strip.show();
        digitalWrite(XLED_PIN,LOW);
        delay(500);        
    }       
}

void loop() 
{
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        button[i].update();

        if (button[i].fell()) {
            strip.clear();
            strip.setPixelColor(i,0,32,32);
            strip.show();
        }
    }
}

// int main() 
// {
//     setup();
//     while (true) {
//         loop();
//     }
// }