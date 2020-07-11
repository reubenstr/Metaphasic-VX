#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include <JC_Button.h>         // https://github.com/JChristensen/JC_Button
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_STRIP_SPORATION_CHAMBER 5
#define PIN_STRIP_STATE_INDICATORS A0
#define PIN_STRIP_GLYPH_INDICATORS A2
#define PIN_STRIP_WARNING_1_INDICATOR A1
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

Adafruit_NeoPixel stripChamber = Adafruit_NeoPixel(7, PIN_STRIP_SPORATION_CHAMBER, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripGlyph = Adafruit_NeoPixel(25, PIN_STRIP_GLYPH_INDICATORS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripStates = Adafruit_NeoPixel(15, PIN_STRIP_STATE_INDICATORS, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripWarning1 = Adafruit_NeoPixel(9, PIN_STRIP_WARNING_1_INDICATOR, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripWarning2 = Adafruit_NeoPixel(9, PIN_STRIP_WARNING_2_INDICATOR, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;
PCA9685 pwmController2;

Button buttonPoly(PIN_BUTTON_POLY);
Button buttonMono(PIN_BUTTON_MONO);

enum SeedState
{
  poly,
  mono
} seedState;

//int nodeMap[10][4] = {{}}

void UpdateGlyphIndicator()
{
  static msTimer timer(1000);
  static bool activeGlyphs[25];

  int delay = state == stable ? 2000 : state == warning ? 1250 : state == critical ? 500 : 0;
  timer.setDelay(delay);

  if (timer.elapsed())
  {
    int fillAmount = state == stable ? 10 : state == warning ? 15 : state == critical ? 20 : 0;

    RandomArrayFill(activeGlyphs, fillAmount, sizeof(activeGlyphs));

    for (int i = 0; i < stripGlyph.numPixels(); i++)
    {
      uint32_t color;
      if (state == critical)
      {
        color = Wheel(random(0, 255));
      }
      else
      {
        int r = random(0, 3);
        color = r == 0 ? 0x00FF0000 : r == 1 ? 0x000000FF : r == 2 ? 0x00FF00 : 0;
      }

      if (activeGlyphs[i])
        stripGlyph.setPixelColor(i, color);
      else
        stripGlyph.setPixelColor(i, 0);
    }
    stripGlyph.show();
  }
}

void UpdateStateIndicators()
{
  static flasher flasher(Pattern::Sin, 1000, 255);

  int fillStart = state == stable ? 0 : state == warning ? 1 : state == critical ? 2 : 0;
  uint32_t color = state == stable ? Color(0, flasher.getPwmValue(), 0) : state == warning ? Color(flasher.getPwmValue() / 2, flasher.getPwmValue() / 2, 0) : state == critical ? Color(flasher.getPwmValue(), 0, 0) : 0;
  Pattern pattern = state == stable ? Pattern::Solid : state == warning ? Pattern::RandomReverseFlash : state == critical ? Pattern::Sin : Pattern::Solid;

  flasher.setPattern(pattern);
  stripStates.fill(0, 0, stripStates.numPixels());
  stripStates.fill(color, fillStart * 5, 5);
  stripStates.show();
}

void UpdateWarningIndicators()
{
  static bool warnings[6];
  static flasher flasherWarnings[6];
  static msTimer timerWarning(5000);

  int delay = state == stable ? 1000 : state == warning ? 750 : state == critical ? 500 : 0;

  if (timerWarning.elapsed())
  {

  int numWarnings = state == stable ? 1 : state == warning ? 2 : state == critical ? 3 : 0;
  numWarnings += random(0, 2);

    RandomArrayFill(warnings, numWarnings, sizeof(warnings));

    for (int i = 0; i < 6; i++)
    {
      flasherWarnings[i].setDelay(delay + random(0, 100));
      flasherWarnings[i].repeat(warnings[i]);
    }
  }

  stripWarning1.fill(Color(flasherWarnings[0].getPwmValue(), 0, 0), 0, 3);
  stripWarning1.fill(Color(flasherWarnings[1].getPwmValue(), 0, 0), 3, 3);
  stripWarning1.fill(Color(flasherWarnings[2].getPwmValue(), 0, 0), 6, 3);
  stripWarning2.fill(Color(flasherWarnings[3].getPwmValue(), 0, 0), 0, 3);
  stripWarning2.fill(Color(flasherWarnings[4].getPwmValue(), 0, 0), 3, 3);
  stripWarning2.fill(Color(flasherWarnings[5].getPwmValue(), 0, 0), 6, 3);
  stripWarning1.show();
  stripWarning2.show();
}

void UpdateChamber()
{

  static msTimer timer(20);
  static flasher flasherWarnings[7];
  static byte wheelPos;

  if (timer.elapsed())
  {
    wheelPos++;
    if (seedState == poly)
    {
      for (int i = 0; i < stripChamber.numPixels(); i++)
      {
        stripChamber.setPixelColor(i, Wheel(((i * 256 / stripChamber.numPixels()) + wheelPos) & 255));
      }
    }
    else if (seedState == mono)
    {
      for (int i = 0; i < stripChamber.numPixels(); i++)
      {
        stripChamber.setPixelColor(i, Wheel(wheelPos));
      }
    }
  }

  // PIN_TOGGLE_ASYNC
  // PIN_TOGGLE_PULSE
  // PIN_TOGGLE_DECAY

  stripChamber.show();
}

void UpdatePWMs()
{
  uint16_t pwms1[16];

  // Update LED segment bar.
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

  // Update buttons and indicators.

  pwms1[4] = intensity > 7 ? maxPwmGenericLed : 0;
  pwms1[2] = seedState == poly ? maxPwmGenericLed : 0;
  pwms1[3] = seedState == mono ? maxPwmGenericLed : 0;

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void UpdateStars()
{

  buttonPoly.read();
  buttonMono.read();

  bool sporationFlag = false;

  if (buttonPoly.wasPressed())
  {
    seedState = poly;
    sporationFlag = true;
  }

  if (buttonMono.wasPressed())
  {
    seedState = mono;
    sporationFlag = true;
  }

  if (sporationFlag)
  {
  }

  uint16_t pwms2[16];
  static int node;
  static msTimer timerNode(1000);
  if (timerNode.elapsed())
  {
    if (++node > 15)
      node = 0;
  }

  for (int i = 0; i < 15; i++)
  {
    pwms2[i] = i == node ? maxPwmBlueLed : 0;
  }

  pwmController2.setChannelsPWM(0, 16, pwms2);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_LED_BACKGROUND, LOW);

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);
  pwmController1.init(0x41);
  pwmController1.setPWMFrequency(1500);

  stripGlyph.begin();
  stripStates.begin();
  stripWarning1.begin();
  stripWarning2.begin();

  buttonPoly.begin();
  buttonMono.begin();
}

void loop()
{

  digitalWrite(PIN_LED_BACKGROUND, false);

  //TEMP
  static msTimer timerState(5000);
  static int stateIndex;
  if (timerState.elapsed())
  {
    if (++stateIndex > 2)
      stateIndex = 0;
    state = (states)stateIndex;
  }

  state = stable;

  UpdateGlyphIndicator();

  UpdateStateIndicators();

  UpdateWarningIndicators();

  //UpdateChamber();

  //UpdatePWMs();

  //UpdateStars();
}