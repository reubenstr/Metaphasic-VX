#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.
#include <LiquidCrystal_I2C.h>


#define PIN_STRIP_GENERATOR 11
#define PIN_STRIP_ROUND_1 10
#define PIN_STRIP_ROUND_2 9
#define PIN_STRIP_ROUND_3 8
#define PIN_STRIP_CIRCLE 7
#define PIN_BUTTON_INJECTION A0
#define PIN_BUTTON_AGITATION A1
#define PIN_BUTTON_SUPRESSION A2
#define PIN_BUTTON_PLUMBUS A3
#define PIN_TOGGLE_BRAKE 13
#define PIN_BUTTOM_CYCLE 12
#define PIN_POT_NANOGAIN A7
#define PIN_POT_CORRECTION A6

#define LCD_COLUMNS 20
#define LCD_ROWS   4
#define PAGE   ((COLUMS) * (ROWS))

Adafruit_NeoPixel stripGenerator = Adafruit_NeoPixel(3, PIN_STRIP_GENERATOR, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex1 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex2 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex3 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripCircle = Adafruit_NeoPixel(44, PIN_STRIP_CIRCLE, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);


void UpdatePWMs()
{
  uint16_t pwms1[16];
  /*
PIN_BUTTON_INJECTION
PIN_BUTTON_AGITATION
PIN_BUTTON_SUPRESSION
PIN_BUTTON_PLUMBUS
*/
  if (!digitalRead(PIN_BUTTON_INJECTION))
    Serial.write(0);
  if (!digitalRead(PIN_BUTTON_AGITATION))
    Serial.write(1);
  if (!digitalRead(PIN_BUTTON_SUPRESSION))
    Serial.write(2);
  if (!digitalRead(PIN_BUTTON_PLUMBUS))
    Serial.write(3);

  pwms1[5] = !digitalRead(PIN_BUTTON_INJECTION) ? 4000 : 0;
  pwms1[6] = !digitalRead(PIN_BUTTON_AGITATION) ? 4000 : 0;
  pwms1[4] = !digitalRead(PIN_BUTTON_SUPRESSION) ? 4000 : 0;
  pwms1[7] = !digitalRead(PIN_BUTTON_PLUMBUS) ? 4000 : 0;

  pwms1[9] = 4095;
  pwms1[10] = map(analogRead(PIN_POT_NANOGAIN), 0, 1023, 0, 4095);
  pwms1[11] = map(analogRead(PIN_POT_CORRECTION), 0, 1023, 0, 4095);

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_BUTTON_INJECTION, INPUT);
  pinMode(PIN_BUTTON_AGITATION, INPUT);
  pinMode(PIN_BUTTON_SUPRESSION, INPUT);
  pinMode(PIN_BUTTON_PLUMBUS, INPUT);

  digitalWrite(PIN_BUTTON_INJECTION, HIGH);
  digitalWrite(PIN_BUTTON_AGITATION, HIGH);
  digitalWrite(PIN_BUTTON_SUPRESSION, HIGH);
  digitalWrite(PIN_BUTTON_PLUMBUS, HIGH);

  stripVortex1.setBrightness(127);
  stripVortex2.setBrightness(127);
  stripVortex3.setBrightness(127);

  stripGenerator.begin();
  stripVortex1.begin();
  stripVortex2.begin();
  stripVortex3.begin();
  stripCircle.begin();

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);

  lcd.begin(LCD_COLUMNS, LCD_ROWS);
}

void loop()
{

  UpdatePWMs();


  lcd.print(F("PCF8574 is OK...")); 

  static flasher flasherGenerator(Pattern::Sin, 1000, 255);
  stripGenerator.fill(stripGenerator.Color(0, flasherGenerator.getPwmValue(), 0), 0, stripGenerator.numPixels());
  stripGenerator.show();

  static msTimer timerVortex1(100);
  if (timerVortex1.elapsed())
  {
    static int x = 0;
    if (++x > 15)
      x = 0;

    for (int i = 0; i < 16; i++)
    {
      uint32_t color;

      if (x == i)
        color = 255;
      else
        color = 0;

      stripVortex1.setPixelColor(i, stripVortex1.Color(0, color, 0));
      stripVortex2.setPixelColor(i, stripVortex2.Color(0, color, 0));
      stripVortex3.setPixelColor(i, stripVortex3.Color(0, color, 0));
    }

    stripVortex1.show();
    stripVortex2.show();
    stripVortex3.show();

    static int y = 0;
    if (++y > stripCircle.numPixels() - 1)
      y = 0;

    for (int i = 0; i < stripCircle.numPixels(); i++)
    {
      uint32_t color;
      if (y == i)
        color = 255;
      else
        color = 0;
      stripCircle.setPixelColor(i, stripCircle.Color(color, 0, 0));
    }

    stripCircle.show();
  }
}