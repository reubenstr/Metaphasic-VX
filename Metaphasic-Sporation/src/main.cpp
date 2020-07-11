#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_STRIP_SPORATION 5
#define PIN_STRIP_MAIN_INDICATOR A0
#define PIN_STRIP_GLYPH_INDICATOR A1
#define PIN_STRIP_WARNING_1_INDICATOR A2
#define PIN_STRIP_WARNING_2_INDICATOR A3
#define PIN_TOGGLE_ASYNC 12
#define PIN_TOGGLE_PULSE 11
#define PIN_TOGGLE_DECAY 10
#define PIN_OFFSET_0 9
#define PIN_OFFSET_0 8
#define PIN_OFFSET_0 7
#define PIN_OFFSET_0 6
#define PIN_BUTTON_POLY 3
#define PIN_BUTTON_MONO 2
#define PIN_LED_BACKGROUND 4

Adafruit_NeoPixel stripGlyph = Adafruit_NeoPixel(25, PIN_STRIP_GLYPH_INDICATOR, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;
PCA9685 pwmController2;

void UpdateGlyphIndicator()
{
  static msTimer timerGlyph(2000);
  if (timerGlyph.elapsed())
  {
    for (int i = 0; i < stripGlyph.numPixels(); i++)
    {
      stripGlyph.setPixelColor(i, Wheel(random(0, 255)));
    }
    stripGlyph.show();
  }
}

void UpdatePWMs()
{
  uint16_t pwms1[16];

static msTimer timerIntensity(1000);
  static int intensity;
  if (timerIntensity.elapsed())
  {
    intensity = random(0, 10);
  }

  for (int i = 6; i < 16; i++)
  {
      pwms1[i] = i - 6 >= intensity ? maxPwmGenericLed : 0;
  }

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void UpdateStars()
{

  uint16_t pwms2[16];

  pwmController2.setChannelsPWM(0, 16, pwms2);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_LED_BACKGROUND, OUTPUT);

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);
  pwmController1.init(0x41);
  pwmController1.setPWMFrequency(1500);
}

void loop()
{

  digitalWrite(PIN_LED_BACKGROUND, HIGH);

  UpdateGlyphIndicator();

  UpdatePWMs();

  UpdateStars();
}