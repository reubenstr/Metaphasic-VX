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
  static msTimer timerRed(100);
  static msTimer timerGreen(100);
  static msTimer timerBlue(100);

  static msTimer redDetachTimer(50);
  static msTimer greenDetachTimer(50);
  static msTimer blueDetachTimer(50);

  int redPos, greenPos, bluePos;
  int delayMs = state == stable ? 4000 : state == warning ? 3000 : state == critical ? 2000 : 0;

  int bias = map(analogRead(PIN_ANALOG_POT_HEISENBERG_BIAS), 0, 1023, 0, 15);

  if (timerRed.elapsed())
  {
    timerRed.setDelay(delayMs + random(0, delayMs));
    redPos = RandomServoPosition();
    if (mode == active)
    {
      servoRed.attach(PIN_SERVO_RED);
      servoRed.write(redPos + bias);
    }
    redDetachTimer.resetDelay();
  }

  if (timerGreen.elapsed())
  {
    timerGreen.setDelay(delayMs + random(0, delayMs));
    greenPos = RandomServoPosition();
    if (mode == active)
    {
      servoGreen.attach(PIN_SERVO_GREEN);
      servoGreen.write(greenPos + bias);
    }
    greenDetachTimer.resetDelay();
  }

  if (timerBlue.elapsed())
  {
    timerBlue.setDelay(delayMs + random(0, delayMs));
    bluePos = RandomServoPosition();
    if (mode == active)
    {
      servoBlue.attach(PIN_SERVO_BLUE);
      servoBlue.write(bluePos + bias);
    }
    blueDetachTimer.resetDelay();
  }

  if (redDetachTimer.elapsed())
    servoRed.detach();
  if (greenDetachTimer.elapsed())
    servoGreen.detach();
  if (blueDetachTimer.elapsed())
    servoBlue.detach();

  int delta = abs(redPos - 90) + abs(greenPos - 90) + abs(bluePos - 90);
  int ledPos = map(delta, 0, 120, 0, 255);
  stripIndicatorLeft.setPixelColor(0, stripIndicatorLeft.Color(ledPos, 255 - ledPos, 0));
  stripIndicatorLeft.show();
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

  state = critical;
  mode = passive;

  int brightnessOffset = map(analogRead(PIN_ANALOG_POT_ATOMIC_TRI_BOND), 0, 1023, 0, 200);
  stripIndicatorLeft.setBrightness(100 - brightnessOffset / 2.5);
  stripIndicatorRight.setBrightness(255 - brightnessOffset);

  UpdateLeftTriangle();

  bool fill = digitalRead(PIN_TOGGLE_2);
  UpdateRightTriangle(fill);
}