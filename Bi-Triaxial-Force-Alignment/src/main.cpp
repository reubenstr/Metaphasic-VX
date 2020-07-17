#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <Servo.h>
#include "common.h"  // Local libary.
#include "msTimer.h" // Local libary.
#include "flasher.h" // Local libary.

#define PIN_ANALOG_POT_HEISENBERG_BIAS A2
#define PIN_ANALOG_POT_ATOMIC_TRI_BOND A6
#define PIN_ANALOG_POT_MIDI_CLORIAN_COMPENSATION A3
#define PIN_LED_POINTER_RED 6
#define PIN_LED_POINTER_GREEN 3
#define PIN_LED_POINTER_BLUE 5
#define PIN_TOGGLE_1 A0
#define PIN_TOGGLE_2 A1
#define PIN_WS2812B_CENTER_INDICATOR_LEFT 12
#define PIN_WS2812B_CENTER_INDICATOR_RIGHT 9
#define PIN_SERVO_RED 2
#define PIN_SERVO_GREEN 4
#define PIN_SERVO_BLUE 7

PCA9685 pwmController1;
PCA9685 pwmController2;

Adafruit_NeoPixel stripIndicatorLeft = Adafruit_NeoPixel(1, PIN_WS2812B_CENTER_INDICATOR_LEFT, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripIndicatorRight = Adafruit_NeoPixel(1, PIN_WS2812B_CENTER_INDICATOR_RIGHT, NEO_RGB + NEO_KHZ800);

Servo servoRed, servoGreen, servoBlue;

signed int trigainDelta, tripanGamma, triphaseBeta;

void UpdatePWMs(bool fill)
{
  uint16_t pwms1[16];
  uint16_t pwms2[16];

  // Fill LED bars with PWM values. LED order is opposite PWM pin order.
  for (int i = 0; i < 10; i++)
  {
    if (fill)
    {
      pwms1[i] = (9 - trigainDelta <= i) ? maxPwmGenericLed : 0;
      pwms2[i + 4] = (9 - triphaseBeta <= i) ? maxPwmGenericLed : 0;
      if (i <= 3)
      {
        pwms2[3 - i] = (9 - tripanGamma <= i) ? maxPwmGenericLed : 0;
      }
      else
      {
        pwms1[19 - i] = (9 - tripanGamma <= i) ? maxPwmGenericLed : 0;
      }
    }
    else
    {
      pwms1[i] = (9 - trigainDelta == i) ? maxPwmGenericLed : 0;
      pwms2[i + 4] = (9 - triphaseBeta == i) ? maxPwmGenericLed : 0;
      if (i <= 3)
      {
        pwms2[3 - i] = (9 - tripanGamma == i) ? maxPwmGenericLed : 0;
      }
      else
      {
        pwms1[19 - i] = (9 - tripanGamma == i) ? maxPwmGenericLed : 0;
      }
    }
  }

  pwmController1.setChannelsPWM(0, 16, pwms1);
  pwmController2.setChannelsPWM(0, 16, pwms2);
}

int RandomServoPosition()
{
  int pos = 90;
  int min = state == stable ? 90 : state == warning ? 80 : state == critical ? 70 : 90;
  int max = state == stable ? 90 : state == warning ? 100 : state == critical ? 110 : 90;

  if (random(0, 2) == 0)
  {
    pos = random(max, max + 20);
  }
  else
  {
    pos = random(min - 20, min);
  }

  return pos;
}

void SetTriServoValues()
{

  static msTimer timerDetach(250);
  int redPos, greenPos, bluePos;

  if (performActivityFlag && digitalRead(PIN_TOGGLE_1))
  {
    performActivityFlag = false;
    timerDetach.resetDelay();

    int bias = map(analogRead(PIN_ANALOG_POT_HEISENBERG_BIAS), 0, 1023, 0, 15);

    redPos = RandomServoPosition();
    greenPos = RandomServoPosition();
    bluePos = RandomServoPosition();

    servoRed.attach(PIN_SERVO_RED);
    servoGreen.attach(PIN_SERVO_GREEN);
    servoBlue.attach(PIN_SERVO_BLUE);

    servoRed.write(redPos + bias);
    servoGreen.write(redPos + bias);
    servoBlue.write(redPos + bias);

    int delta = abs(redPos - 90) + abs(greenPos - 90) + abs(bluePos - 90);
    int ledPos = map(delta, 0, 120, 0, 255);
    stripIndicatorLeft.setPixelColor(0, stripIndicatorLeft.Color(ledPos, 255 - ledPos, 0));
    stripIndicatorLeft.show();
  }
  else
  {
    if (timerDetach.elapsed())
    {
      servoRed.detach();
      servoGreen.detach();
      servoBlue.detach();
    }
  }
}

void SetTriValues()
{
  static msTimer timer(100);
  static signed int trigainDeltaTarget, tripanGammaTarget, triphaseBetaTarget;

  int delay = state == stable ? 60 : state == warning ? 30 : state == critical ? 10 : 0;
  delay += map(analogRead(PIN_ANALOG_POT_MIDI_CLORIAN_COMPENSATION), 0, 1023, 0, 50);

  timer.setDelay(delay);

  if (timer.elapsed())
  {
    int minRange = state == stable ? 1 : state == warning ? 3 : state == critical ? 7 : 0;
    int maxRange = state == stable ? 6 : state == warning ? 8 : state == critical ? 10 : 0;

    if (trigainDelta == trigainDeltaTarget)
    {
      trigainDeltaTarget = random(minRange, maxRange);
    }
    else
    {
      if (trigainDelta > trigainDeltaTarget)
        trigainDelta--;
      if (trigainDelta < trigainDeltaTarget)
        trigainDelta++;
    }

    if (tripanGamma == tripanGammaTarget)
    {
      tripanGammaTarget = random(minRange, maxRange);
    }
    else
    {
      if (tripanGamma > tripanGammaTarget)
        tripanGamma--;
      if (tripanGamma < tripanGammaTarget)
        tripanGamma++;
    }

    if (triphaseBeta == triphaseBetaTarget)
    {
      triphaseBetaTarget = random(minRange, maxRange);
    }
    else
    {
      if (triphaseBeta > triphaseBetaTarget)
        triphaseBeta--;
      if (triphaseBeta < triphaseBetaTarget)
        triphaseBeta++;
    }
  }
}

void UpdateLeftTriangle()
{

  SetTriServoValues();

  static flasher flasherRed(Pattern::Sin, 1000, 255);
  static flasher flasherGreen(Pattern::Sin, 1000, 255);
  static flasher flasherBlue(Pattern::Sin, 1000, 255);

  if (digitalRead(PIN_TOGGLE_1))
  {
    flasherRed.setPattern(Pattern::Sin);
    flasherGreen.setPattern(Pattern::Sin);
    flasherBlue.setPattern(Pattern::Sin);
  }
  else
  {
    flasherRed.setPattern(Pattern::RandomFlash);
    flasherGreen.setPattern(Pattern::RandomFlash);
    flasherBlue.setPattern(Pattern::RandomFlash);
  }

  analogWrite(PIN_LED_POINTER_RED, flasherRed.getPwmValue());
  analogWrite(PIN_LED_POINTER_GREEN, flasherGreen.getPwmValue());
  analogWrite(PIN_LED_POINTER_BLUE, flasherBlue.getPwmValue());
}

void UpdateRightTriangle(bool fill)
{
  SetTriValues();

  UpdatePWMs(fill);

  int total = trigainDelta + tripanGamma + triphaseBeta;

  if (total <= 6)
  {
    stripIndicatorRight.setPixelColor(0, stripIndicatorRight.Color(0, 0, 255));
  }
  else if (total <= 15)
  {
    stripIndicatorRight.setPixelColor(0, stripIndicatorRight.Color(0, 255, 0));
  }
  else if (total <= 24)
  {
    stripIndicatorRight.setPixelColor(0, stripIndicatorRight.Color(127, 127, 0));
  }
  else
  {
    stripIndicatorRight.setPixelColor(0, stripIndicatorRight.Color(255, 0, 0));
  }
  stripIndicatorRight.show();
}

void CheckActivity()
{
  static int oldPot1, pot1;
  static int oldPot2, pot2;
  static int oldPot3, pot3;
  const int rangeTest = 2;

  pot1 = (map(analogRead(PIN_ANALOG_POT_HEISENBERG_BIAS), 0, 1024, 0, 10));
  pot2 = (map(analogRead(PIN_ANALOG_POT_ATOMIC_TRI_BOND), 0, 1024, 0, 10));
  pot3 = (map(analogRead(PIN_ANALOG_POT_MIDI_CLORIAN_COMPENSATION), 0, 1024, 0, 10));

  if (!InRange(pot1, oldPot1 - rangeTest, oldPot1 + rangeTest))
  {
    oldPot1 = pot1;
    activityFlag = true;
  }

  if (!InRange(pot2, oldPot2 - rangeTest, oldPot2 + rangeTest))
  {
    oldPot2 = pot2;
    activityFlag = true;
  }

  if (!InRange(pot3, oldPot3 - rangeTest, oldPot3 + rangeTest))
  {
    oldPot3 = pot3;
    activityFlag = true;
  }

  static bool oldToggle1, toggle1;
  static bool oldToggle2, toggle2;

  toggle1 = digitalRead(PIN_TOGGLE_1);
  toggle2 = digitalRead(PIN_TOGGLE_2);

  if (oldToggle1 != toggle1)
  {
    oldToggle1 = toggle1;
    activityFlag = true;
  }

  if (oldToggle2 != toggle2)
  {
    oldToggle2 = toggle2;
    activityFlag = true;
  }
}

void setup()
{
  delay(500);
  Serial.begin(BAUD_RATE);

  pinMode(PIN_TOGGLE_1, INPUT);
  digitalWrite(PIN_TOGGLE_1, HIGH);
  pinMode(PIN_TOGGLE_2, INPUT);
  digitalWrite(PIN_TOGGLE_2, HIGH);

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);

  pwmController2.init(0x41);
  pwmController2.setPWMFrequency(1500);

  stripIndicatorLeft.begin();
  stripIndicatorRight.begin();
}

void loop()
{

  CheckControlData();

  int brightnessOffset = map(analogRead(PIN_ANALOG_POT_ATOMIC_TRI_BOND), 0, 1023, 0, 200);
  stripIndicatorLeft.setBrightness(100 - brightnessOffset / 2.5);
  stripIndicatorRight.setBrightness(255 - brightnessOffset);

  UpdateLeftTriangle();

  bool fill = digitalRead(PIN_TOGGLE_2);
  UpdateRightTriangle(fill);

  CheckActivity();
}