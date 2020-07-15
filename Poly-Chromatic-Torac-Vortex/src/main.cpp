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
#define PIN_STRIP_ROUND_1 8
#define PIN_STRIP_ROUND_2 9
#define PIN_STRIP_ROUND_3 10
#define PIN_STRIP_CIRCLE 7
#define PIN_BUTTON_INJECTION A0
#define PIN_BUTTON_AGITATION A1
#define PIN_BUTTON_SUPRESSION A2
#define PIN_BUTTON_PLUMBUS A3
#define PIN_TOGGLE_BRAKE 13
#define PIN_BUTTOM_CYCLE 12
#define PIN_POT_NANOGAIN A6
#define PIN_POT_CORRECTION A7
#define PIN_MOTOR 2

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

struct TuningValues
{
  int nanogain;
  int correction;
  signed int oldNanogain;
  signed int oldCorrection;
  bool updateFlag;
} tuningValues;

const int vertexBaseSpeed = 160;

void UpdatePWMs()
{
  uint16_t pwms1[16];

  pwms1[5] = controlStates.injection ? maxPwmGenericLed : 0;
  pwms1[6] = controlStates.agitation ? maxPwmGenericLed : 0;
  pwms1[4] = controlStates.supression ? maxPwmGenericLed : 0;
  pwms1[7] = controlStates.plumbus ? maxPwmGenericLed : 0;

  static flasher flasherIndicator(Pattern::Sin, 750, maxPwmRedLed);
  pwms1[9] = state == warning ? flasherIndicator.getPwmValue() : 0;

  pwms1[10] = tuningValues.nanogain > 13 || tuningValues.correction > 25 ? maxPwmRedLed : 0;

  static flasher flasherStraightened(Pattern::Solid, 500, maxPwmRedLed);
  pwms1[11] = state == warning || state == critical ? flasherStraightened.getPwmValue() : 0;

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void UpdateLcdDisplay()
{
  static msTimer timerNewMessage(1000);
  static msTimer timerLcdRefresh(100);
  static int message = 0;
  const int numMessages = 4;

  buttonCycle.read();

  if (buttonCycle.wasPressed())
  {
    if (++message > numMessages)
    {
      message = 0;
    }
    activityFlag = true;
  }

  if (timerNewMessage.elapsed())
  {
    timerNewMessage.setDelay(2000);
    if (mode == passive)
    {
      message = random(0, numMessages);
    }
  }

  if (timerLcdRefresh.elapsed() || tuningValues.updateFlag)
  {

    if (tuningValues.updateFlag)
    {
      timerNewMessage.setDelayAndReset(5000);
      tuningValues.updateFlag = false;
      message = 1;
    }

    lcd.setCursor(0, 0);

    if (message == 0)
    {
      lcd.print(F("PCF8574 is OK.  "));
      lcd.setCursor(0, 1);
      lcd.print(F("PCA9685 is OK.  "));
    }

    if (message == 1)
    {
      char buf[20];
      sprintf(buf, "Nanogain: %02unX/s", tuningValues.nanogain);
      lcd.print(buf);
      lcd.setCursor(0, 1);
      sprintf(buf, "C. Corr.: %02upY/m", tuningValues.correction);
      lcd.print(buf);
    }

    if (message == 2)
    {
      lcd.print(F("Morty status:   "));
      lcd.setCursor(0, 1);
      if (state == stable)
        lcd.print(F("Mellow          "));
      if (state == warning)
        lcd.print(F("Annoying        "));
      if (state == critical)
        lcd.print(F("Freaking out!   "));
    }

    if (message == 3)
    {
      static int abvPercent = 15;
      static msTimer timerAbv(2000);
      if (timerAbv.elapsed())
        abvPercent = random(25, 30);
      lcd.print(F("*** Warning *** "));
      lcd.setCursor(0, 1);
      lcd.print(F("ABV below 0."));
      lcd.print(abvPercent, DEC);
      lcd.print(F("%!"));
    }
  }
}

void CheckButtons()
{
  buttonInjection.read();
  buttonAgitation.read();
  buttonSuppression.read();
  buttonPlumbus.read();

  if (buttonInjection.wasPressed())
  {
    controlStates.injection = !controlStates.injection;

    if (!controlStates.injection)
    {
      controlStates.agitation = false;
    }
    activityFlag = true;
  }

  if (buttonAgitation.wasPressed())
  {
    controlStates.agitation = !controlStates.agitation;

    if (controlStates.agitation)
    {
      controlStates.injection = true;
    }
    activityFlag = true;
  }

  if (buttonSuppression.wasPressed())
  {
    controlStates.supression = !controlStates.supression;
    activityFlag = true;
  }

  if (buttonPlumbus.wasPressed())
  {
    controlStates.plumbus = !controlStates.plumbus;
    activityFlag = true;
  }
}

void CheckPotentiometers()
{
  tuningValues.nanogain = map(analogRead(PIN_POT_NANOGAIN), 0, 1023, 0, 15);
  tuningValues.correction = map(analogRead(PIN_POT_CORRECTION), 0, 1023, 0, 30);

  if (tuningValues.nanogain > tuningValues.oldNanogain + 1 ||
      tuningValues.nanogain < tuningValues.oldNanogain - 1)
  {
    tuningValues.updateFlag = true;
    tuningValues.oldNanogain = tuningValues.nanogain;
    activityFlag = true;
  }

  if (tuningValues.correction > tuningValues.oldCorrection + 1 ||
      tuningValues.correction < tuningValues.oldCorrection - 1)
  {
    tuningValues.updateFlag = true;
    tuningValues.oldCorrection = tuningValues.correction;
    activityFlag = true;
  }
}

void UpdateVortexStrip1()
{
  static msTimer timerVortex(100);
  static int pixelIndex;
  static byte wheelVortex;
  int pixelOffset = 0;

  if (timerVortex.elapsed())
  {
    timerVortex.setDelay(vertexBaseSpeed - (tuningValues.nanogain * 10));

    if (controlStates.agitation)
    {
      wheelVortex = random(0, 256);
    }
    else
    {
      wheelVortex += 5;
    }

    if (++pixelIndex > stripVortex1.numPixels() - 1)
    {
      pixelIndex = 0;
    }

    for (int i = 0; i < stripVortex1.numPixels(); i++)
    {

      int pixelIndexAfterOffset = !controlStates.plumbus ? RollOverValue(pixelIndex, pixelOffset, 0, stripVortex1.numPixels()) : pixelIndex;

      if (i == pixelIndexAfterOffset)
      {
        if (controlStates.injection)
        {
          // Rainbow color.
          //stripVortex2.setPixelColor(i, Wheel(((i * 256 / stripVortex2.numPixels()) + wheelVortex2) & 255));
          stripVortex1.setPixelColor(i, Wheel(wheelVortex));
        }
        else
        {
          // Solid color.
          stripVortex1.setPixelColor(i, Color(255, 0, 0));
        }
      }
      else
      {
        if (controlStates.supression)
        {
          stripVortex1.setPixelColor(i, 0);
        }
        else
        {
          stripVortex1.setPixelColor(i, Fade(stripVortex1.getPixelColor(i), 50));
        }
      }
    }
    stripVortex1.show();
  }
}

void UpdateVortexStrip2()
{
  static msTimer timerVortex(100);
  static int pixelIndex;
  static byte wheelVortex;
  int pixelOffset = 5;

  if (timerVortex.elapsed())
  {
    timerVortex.setDelay(vertexBaseSpeed - (tuningValues.nanogain * 10));

    if (controlStates.agitation)
    {
      wheelVortex = random(0, 256);
    }
    else
    {
      wheelVortex += 5;
    }

    if (++pixelIndex > stripVortex2.numPixels() - 1)
    {
      pixelIndex = 0;
    }

    for (int i = 0; i < stripVortex2.numPixels(); i++)
    {
      int pixelIndexAfterOffset = !controlStates.plumbus ? RollOverValue(pixelIndex, pixelOffset, 0, stripVortex1.numPixels()) : pixelIndex;

      if (i == pixelIndexAfterOffset)
      {
        if (controlStates.injection)
        {
          // Rainbow color.
          //stripVortex2.setPixelColor(i, Wheel(((i * 256 / stripVortex2.numPixels()) + wheelVortex2) & 255));
          stripVortex2.setPixelColor(i, Wheel(wheelVortex));
        }
        else
        {
          // Solid color.
          stripVortex2.setPixelColor(i, Color(0, 255, 0));
        }
      }
      else
      {
        if (controlStates.supression)
        {
          stripVortex2.setPixelColor(i, 0);
        }
        else
        {
          stripVortex2.setPixelColor(i, Fade(stripVortex2.getPixelColor(i), 50));
        }
      }
    }
    stripVortex2.show();
  }
}

void UpdateVortexStrip3()
{
  static msTimer timerVortex(100);
  static int pixelIndex;
  static byte wheelVortex;
  int pixelOffset = 10;

  if (timerVortex.elapsed())
  {
    timerVortex.setDelay(vertexBaseSpeed - (tuningValues.nanogain * 10));

    if (controlStates.agitation)
    {
      wheelVortex = random(0, 256);
    }
    else
    {
      wheelVortex += 5;
    }

    if (++pixelIndex > stripVortex3.numPixels() - 1)
    {
      pixelIndex = 0;
    }

    for (int i = 0; i < stripVortex3.numPixels(); i++)
    {
      int pixelIndexAfterOffset = !controlStates.plumbus ? RollOverValue(pixelIndex, pixelOffset, 0, stripVortex1.numPixels()) : pixelIndex;

      if (i == pixelIndexAfterOffset)
      {
        if (controlStates.injection)
        {
          // Rainbow color.
          //stripVortex2.setPixelColor(i, Wheel(((i * 256 / stripVortex2.numPixels()) + wheelVortex2) & 255));
          stripVortex3.setPixelColor(i, Wheel(wheelVortex));
        }
        else
        {
          // Solid color.
          stripVortex3.setPixelColor(i, Color(0, 0, 255));
        }
      }
      else
      {
        if (controlStates.supression)
        {
          stripVortex3.setPixelColor(i, 0);
        }
        else
        {
          stripVortex3.setPixelColor(i, Fade(stripVortex3.getPixelColor(i), 50));
        }
      }
    }
    stripVortex3.show();
  }
}

void UpdateVertexAll()
{
  static flasher flasherVertex(Pattern::Sin, 500, 255);
  static flasher flasherVertex1(Pattern::RandomFlash, 500, 255);
  static flasher flasherVertex2(Pattern::RandomFlash, 500, 255);
  static flasher flasherVertex3(Pattern::RandomFlash, 500, 255);

  if (state == warning)
  {
    flasherVertex1.setDelay(900);
    flasherVertex2.setDelay(1000);
    flasherVertex3.setDelay(1100);
    flasherVertex1.setPattern(Pattern::Sin);
    flasherVertex2.setPattern(Pattern::Sin);
    flasherVertex3.setPattern(Pattern::Sin);
  }
  else if (state == critical)
  {
    flasherVertex1.setDelay(500);
    flasherVertex2.setDelay(500);
    flasherVertex3.setDelay(500);
    flasherVertex1.setPattern(Pattern::RandomFlash);
    flasherVertex2.setPattern(Pattern::RandomFlash);
    flasherVertex3.setPattern(Pattern::RandomFlash);
  }

  static int oldPwmValue1;
  if (oldPwmValue1 != flasherVertex1.getPwmValue())
  {
    oldPwmValue1 = flasherVertex1.getPwmValue();
    stripVortex1.fill(Color(flasherVertex1.getPwmValue(), 0, 0), 0, stripVortex1.numPixels());
    stripVortex1.show();
  }

  static int oldPwmValue2;
  if (oldPwmValue2 != flasherVertex2.getPwmValue())
  {
    oldPwmValue2 = flasherVertex2.getPwmValue();
    stripVortex2.fill(Color(0, flasherVertex2.getPwmValue(), 0), 0, stripVortex2.numPixels());
    stripVortex2.show();
  }

  static int oldPwmValue3;
  if (oldPwmValue3 != flasherVertex3.getPwmValue())
  {
    oldPwmValue3 = flasherVertex3.getPwmValue();
    stripVortex3.fill(Color(0, 0, flasherVertex3.getPwmValue()), 0, stripVortex3.numPixels());
    stripVortex3.show();
  }

  //stripVortex2.fill(Color(0, flasherVertex2.getPwmValue(), 0), 0, stripVortex1.numPixels());
  // stripVortex3.fill(Color(0, 0, flasherVertex3.getPwmValue()), 0, stripVortex1.numPixels());

  // stripVortex2.show();
  //stripVortex3.show();
}

void UpdateGenerator()
{
  static flasher flasherGenerator(Pattern::Sin, 1000, 255);
  flasherGenerator.setDelay(2000 - (tuningValues.nanogain * 100));
  stripGenerator.fill(stripGenerator.Color(0, 0, flasherGenerator.getPwmValue()), 0, stripGenerator.numPixels());
  stripGenerator.show();
}

void UpdateControlStates()
{
  static msTimer timerActive(3000);
  if (timerActive.elapsed())
  {

    controlStates.injection = random(0, 2) == 0 ? true : false;

    if (controlStates.injection)
    {
      controlStates.agitation = random(0, 2) == 0 ? true : false;
    }
    else
    {
      controlStates.agitation = false;
    }

    controlStates.supression = random(0, 2) == 0 ? true : false;
    controlStates.plumbus = random(0, 2) == 0 ? true : false;
  }
}

void CheckToggle()
{
  static bool oldBreakState;
  bool breakState = digitalRead(PIN_TOGGLE_BRAKE);

  if (oldBreakState != breakState)
  {
    oldBreakState = breakState;
    activityFlag = true;
  }
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);

  buttonCycle.begin();
  buttonInjection.begin();
  buttonAgitation.begin();
  buttonSuppression.begin();
  buttonPlumbus.begin();

  stripVortex1.setBrightness(65);
  stripVortex2.setBrightness(65);
  stripVortex3.setBrightness(65);

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

  // TEMP
  state = stable;
  // TEMP
  mode = active;


  analogWrite(PIN_MOTOR, map(analogRead(PIN_POT_CORRECTION), 0, 1023, 0, 255));

  CheckButtons();

  CheckPotentiometers();

  UpdatePWMs();

  UpdateLcdDisplay();

  UpdateGenerator();

  if (state == stable)
  {
    UpdateVortexStrip1();
    UpdateVortexStrip2();
    UpdateVortexStrip3();
  }
  else
  {
    UpdateVertexAll();
  }

  if (mode == passive)
  {
    UpdateControlStates();
  }

  static msTimer timerCircle(5);
  static byte wheelPos;
  if (timerCircle.elapsed())
  {
    stripCircle.setBrightness(127);
    wheelPos++;
    for (int i = 0; i < stripCircle.numPixels(); i++)
    {
      stripCircle.setPixelColor(i, Wheel(((i * 256 / stripCircle.numPixels()) + wheelPos) & 255));
    }
    stripCircle.show();
  }
}