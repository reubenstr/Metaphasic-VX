#include <Arduino.h>           // PlatformIO
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include <JC_Button.h>         // https://github.com/JChristensen/JC_Button
#include <LiquidCrystal_I2C.h> // https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

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
#define LCD_ROWS 4
#define PAGE ((COLUMS) * (ROWS))

Adafruit_NeoPixel stripGenerator = Adafruit_NeoPixel(3, PIN_STRIP_GENERATOR, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex1 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex2 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripVortex3 = Adafruit_NeoPixel(16, PIN_STRIP_ROUND_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripCircle = Adafruit_NeoPixel(44, PIN_STRIP_CIRCLE, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

Button buttonCycle(PIN_BUTTOM_CYCLE);
Button buttonInjection(PIN_BUTTON_INJECTION);
Button buttonAgitation(PIN_BUTTON_AGITATION);
Button buttonSuppression(PIN_BUTTON_SUPRESSION);
Button buttonPlumbus(PIN_BUTTON_PLUMBUS);

struct ControlStates
{
  bool injection;
  bool agitation;
  bool supression;
  bool plumbus;
} controlStates;

void UpdatePWMs()
{
  uint16_t pwms1[16];



  pwms1[5] = controlStates.injection ? maxPwmGenericLed : 0;
  pwms1[6] = controlStates.agitation ? maxPwmGenericLed : 0;
  pwms1[4] = controlStates.supression ? maxPwmGenericLed : 0;
  pwms1[7] = controlStates.plumbus ? maxPwmGenericLed : 0;

  pwms1[9] = 4095;
  pwms1[10] = map(analogRead(PIN_POT_NANOGAIN), 0, 1023, 0, 4095);
  pwms1[11] = map(analogRead(PIN_POT_CORRECTION), 0, 1023, 0, 4095);

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void UpdateLcdDisplay()
{
  static msTimer timerLcd(1000);
  static int message, oldMessage;
  const int numMessages = 4;

  buttonCycle.read();
  if (buttonCycle.wasPressed())
  {
    if (++message > numMessages)
      message = 0;
  }

  if (timerLcd.elapsed())
  {
    if (mode = active)
    {
      int message = random(0, numMessages);
    }
  }

  if (oldMessage != message)
  {
    oldMessage = message;
    lcd.setCursor(0, 0);

    if (message == 0)
      lcd.print(F("PCF8574 is OK.  "));
    if (message == 1)
    {
      int gain = random(60, 100);
      lcd.print(F("Hypergain:    "));
      lcd.print(gain, DEC);
      lcd.print(F("%"));
    }

    if (message == 2)
      lcd.print(F("Morty calm.     "));
    if (message == 3)
      lcd.print(F("Toraq laser active."));
  }
}

void CheckButtons()
{

  buttonInjection.read();
  buttonAgitation.read();
  buttonSuppression.read();
  buttonPlumbus.read();

  if (buttonInjection.wasPressed())
    controlStates.injection = !controlStates.injection;
  if (buttonAgitation.wasPressed())
    controlStates.agitation = !controlStates.agitation;
  if (buttonSuppression.wasPressed())
    controlStates.supression = !controlStates.supression;
  if (buttonPlumbus.wasPressed())
    controlStates.plumbus = !controlStates.plumbus;
}

void setup()
{
  Serial.begin(BAUD_RATE);

  /*
  pinMode(PIN_BUTTON_INJECTION, INPUT);
  pinMode(PIN_BUTTON_AGITATION, INPUT);
  pinMode(PIN_BUTTON_SUPRESSION, INPUT);
  pinMode(PIN_BUTTON_PLUMBUS, INPUT);

  digitalWrite(PIN_BUTTON_INJECTION, HIGH);
  digitalWrite(PIN_BUTTON_AGITATION, HIGH);
  digitalWrite(PIN_BUTTON_SUPRESSION, HIGH);
  digitalWrite(PIN_BUTTON_PLUMBUS, HIGH);
  */

  buttonCycle.begin();
  buttonInjection.begin();
  buttonAgitation.begin();
  buttonSuppression.begin();
  buttonPlumbus.begin();

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

CheckButtons();

  UpdatePWMs();

  UpdateLcdDisplay();

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