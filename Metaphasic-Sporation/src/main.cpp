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
#define PIN_OFFSET_1 8
#define PIN_OFFSET_2 7
#define PIN_OFFSET_3 6
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

bool genesisFlag;
int fxOffset;

enum SeedState
{
  poly,
  mono
} seedState;

// Map of the connections between the nodes where the node index is the pin on the pwm module.
const signed int nodeMap[15][4] = {{1, -1, -1, -1},
                                   {0, 12, -1, -1},
                                   {8, -1, -1, -1},
                                   {12, -1, -1, -1},
                                   {11, 8, -1, -1},
                                   {11, 6, -1, -1},
                                   {5, -1, -1, -1},
                                   {13, 12, 11, -1},
                                   {2, 10, 4, -1},   // 8
                                   {10, -1, -1, -1}, // 9
                                   {8, 9, -1, -1},   // 10
                                   {14, 7, 5, 4},    // 11
                                   {3, 1, 7, -1},
                                   {7, -1, -1, -1},
                                   {11, -1, -1, -1}};

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
  static msTimer timerWarning(3000);

  int delayTimer = state == stable ? 5000 : state == warning ? 3000 : state == critical ? 2000 : 0;
  int delayFlash = state == stable ? 1000 : state == warning ? 750 : state == critical ? 500 : 0;

  if (timerWarning.elapsed())
  {
    timerWarning.setDelay(delayTimer);

    int numWarnings = state == stable ? 1 : state == warning ? 2 : state == critical ? 3 : 0;
    numWarnings = random(0, numWarnings + 1);

    RandomArrayFill(warnings, numWarnings, sizeof(warnings));

    for (int i = 0; i < 6; i++)
    {
      flasherWarnings[i].setPattern(Pattern::Solid);
      flasherWarnings[i].setDelay(delayFlash + random(0, 100));
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
  static byte wheelPos;

  if (timer.elapsed())
  {
    wheelPos++;
    if (seedState == poly)
    {
      for (signed int i = 0; i < stripChamber.numPixels(); i++)
      {
        stripChamber.setPixelColor(i, Wheel(((i * 256 / stripChamber.numPixels()) + wheelPos) & 255));
      }
    }
    else if (seedState == mono)
    {
      for (signed int i = 0; i < stripChamber.numPixels(); i++)
      {
        stripChamber.setPixelColor(i, Wheel(wheelPos));
      }
    }
  }

  digitalRead(PIN_TOGGLE_ASYNC);

  // PIN_TOGGLE_ASYNC
  // PIN_TOGGLE_PULSE
  // PIN_TOGGLE_DECAY

  stripChamber.show();
}

void UpdatePWMs()
{
  uint16_t pwms1[16];

  // Update LED segment bar.
  static msTimer timerIntensity(250);
  static int targetIntensity;
  static int currentIntensity;
  int minIntensity = state == stable ? 0 : state == warning ? 3 : state == critical ? 6 : 0;

  if (timerIntensity.elapsed())
  {
    if (currentIntensity == targetIntensity)
    {
      targetIntensity = random(minIntensity, 10);
    }
    else
    {
      if (currentIntensity < targetIntensity)
        currentIntensity++;
      if (currentIntensity > targetIntensity)
        currentIntensity--;
    }
  }

  pwms1[6] = 0 >= 9 - currentIntensity ? 1500 : 0;
  pwms1[7] = 1 >= 9 - currentIntensity ? 1500 : 0;
  pwms1[8] = 2 >= 9 - currentIntensity ? 4095 : 0;
  pwms1[9] = 3 >= 9 - currentIntensity ? 4095 : 0;
  pwms1[10] = 4 >= 9 - currentIntensity ? 4095 : 0;
  pwms1[11] = 5 >= 9 - currentIntensity ? 800 : 0;
  pwms1[12] = 6 >= 9 - currentIntensity ? 800 : 0;
  pwms1[13] = 7 >= 9 - currentIntensity ? 800 : 0;
  pwms1[14] = 8 >= 9 - currentIntensity ? 800 : 0;
  pwms1[15] = 9 >= 9 - currentIntensity ? 4095 : 0;

  // Update buttons and indicators.
  static flasher flasherGenensis(Pattern::Sin, 1000, maxPwmBlueLed);
  flasherGenensis.repeat(false);
  if (genesisFlag)
  {
    genesisFlag = false;
    flasherGenensis.reset();
  }

  pwms1[5] = flasherGenensis.getPwmValue();
  pwms1[4] = maxPwmGenericLed; //currentIntensity > 7 ? maxPwmGenericLed : 0;
  pwms1[3] = seedState == mono ? maxPwmGenericLed : 0;
  pwms1[2] = seedState == poly ? maxPwmGenericLed : 0;

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void UpdateStars()
{

  static msTimer timerNode(3000);
  static msTimer timerGenesis(1000);
  static bool nodeStates[15];
  uint16_t pwms2[16];
  bool nodeStatesUpdates[15];
  int delayGenesis = state == stable ? 20000 : state == warning ? 15000 : state == critical ? 10000 : 10000;
  int delaySpread = state == stable ? 3000 : state == warning ? 2000 : state == critical ? 1000 : 10000;
  timerNode.setDelay(delaySpread);

  buttonPoly.read();
  buttonMono.read();

  if (buttonPoly.wasPressed())
  {
    seedState = poly;
    genesisFlag = true;
    activityFlag = true;
  }

  if (buttonMono.wasPressed())
  {
    seedState = mono;
    genesisFlag = true;
    activityFlag = true;
  }

  // Start genesis on a timer.
  if (timerGenesis.elapsed())
  {
    timerGenesis.setDelay(delayGenesis);
    genesisFlag = true;
  }

  // Reset nodes and seed genesis.
  if (genesisFlag)
  {
    for (int i = 0; i < 15; i++)
    {
      nodeStates[i] = false;
    }
    nodeStates[random(0, 16)] = true;
  }

  for (int i = 0; i < 15; i++)
  {
    nodeStatesUpdates[i] = false;
  }

  // Activate adjacent nodes of active nodes.
  if (timerNode.elapsed())
  {
    for (int i = 0; i < 15; i++)
    {
      if (nodeStates[i])
      {
        for (int j = 0; j < 4; j++)
        {
          if (nodeMap[i][j] != -1)
          {
            nodeStatesUpdates[nodeMap[i][j]] = true;
          }
        }
      }
    }

    for (int i = 0; i < 15; i++)
    {
      if (nodeStatesUpdates[i])
      {
        nodeStates[i] = true;
      }
    }
  }

  static flasher flasherNodes[15];
  Pattern pattern = fxOffset == 0 ? Pattern::Sin : fxOffset == 1 ? Pattern::RampUp : fxOffset == 2 ? Pattern::OnOff : fxOffset == 3 ? Pattern::RandomFlash : Pattern::Sin;

  for (int i = 0; i < 15; i++)
  {
    flasherNodes[i].setPattern(pattern);
    pwms2[i] = nodeStates[i] ? flasherNodes[i].getPwmValue() : 0;
  }

  pwmController2.setChannelsPWM(0, 16, pwms2);
}

void CheckFxOffset()
{
  fxOffset = digitalRead(PIN_OFFSET_0) + digitalRead(PIN_OFFSET_1) + digitalRead(PIN_OFFSET_2) + digitalRead(PIN_OFFSET_3);

  static int oldFxOffset;
  if (oldFxOffset != fxOffset)
  {
    oldFxOffset = fxOffset;
    activityFlag = true;
  }
}

void CheckToggles()
{
  int toggleValue = digitalRead(PIN_TOGGLE_ASYNC) + digitalRead(PIN_TOGGLE_PULSE) + digitalRead(PIN_TOGGLE_DECAY);

  static int oldToggleValue;
  if (oldToggleValue != toggleValue)
  {
    oldToggleValue = toggleValue;
    activityFlag = true;
  }
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_OFFSET_0, INPUT);
  pinMode(PIN_OFFSET_1, INPUT);
  pinMode(PIN_OFFSET_2, INPUT);
  pinMode(PIN_OFFSET_3, INPUT);
  digitalWrite(PIN_OFFSET_0, HIGH);
  digitalWrite(PIN_OFFSET_1, HIGH);
  digitalWrite(PIN_OFFSET_2, HIGH);
  digitalWrite(PIN_OFFSET_3, HIGH);

  pinMode(PIN_TOGGLE_ASYNC, INPUT);
  pinMode(PIN_TOGGLE_PULSE, INPUT);
  pinMode(PIN_TOGGLE_DECAY, INPUT);
  digitalWrite(PIN_TOGGLE_ASYNC, HIGH);
  digitalWrite(PIN_TOGGLE_PULSE, HIGH);
  digitalWrite(PIN_TOGGLE_DECAY, HIGH);

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x00);
  pwmController1.setPWMFrequency(1500);
  pwmController2.init(0x01);
  pwmController2.setPWMFrequency(1500);

  stripChamber.begin();
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

  UpdateChamber();

  UpdatePWMs();

  UpdateStars();

  CheckFxOffset();

  CheckToggles();
}