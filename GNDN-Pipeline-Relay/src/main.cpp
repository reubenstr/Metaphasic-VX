#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_STRIP_ISOLINEAR_MANAFOLD 13
//#define PIN_STRIP_DYNAMIC_MULTIPLEX A1
#define PIN_STRIP_SYNAPTIC_GENERATOR A0
#define PIN_STRIP_RADIATION 12
#define PIN_STRIP_GLYPH_INDICATOR 8

#define PIN_RELAY_LEFT_1 A3
#define PIN_RELAY_LEFT_2 A2
#define PIN_RELAY_RIGHT_1 10
#define PIN_RELAY_RIGHT_2 A1

#define PIN_DECODER_S0 2
#define PIN_DECODER_S1 3
#define PIN_DECODER_S2 4
#define PIN_DECODER_S3 5
#define PIN_DECORDER_SIG A6

#define PIN_LED_DISPLAY_1_CLK 6
#define PIN_LED_DISPLAY_2_CLK 6
#define PIN_LED_DISPLAY_3_CLK 6
#define PIN_LED_DISPLAY_1_DIO 7
#define PIN_LED_DISPLAY_2_DIO 9
#define PIN_LED_DISPLAY_3_DIO 11

Adafruit_NeoPixel stripManifold = Adafruit_NeoPixel(3 + 3, PIN_STRIP_ISOLINEAR_MANAFOLD, NEO_GRB + NEO_KHZ800);
//Adafruit_NeoPixel stripMultiplex = Adafruit_NeoPixel(3, PIN_STRIP_DYNAMIC_MULTIPLEX, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripGenerator = Adafruit_NeoPixel(3, PIN_STRIP_SYNAPTIC_GENERATOR, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRadiation = Adafruit_NeoPixel(12, PIN_STRIP_RADIATION, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripGlyphs = Adafruit_NeoPixel(3, PIN_STRIP_GLYPH_INDICATOR, NEO_RGB + NEO_KHZ800);

PCA9685 pwmController1;

TM1637Display ledDisplay1(PIN_LED_DISPLAY_1_CLK, PIN_LED_DISPLAY_1_DIO);
TM1637Display ledDisplay2(PIN_LED_DISPLAY_2_CLK, PIN_LED_DISPLAY_2_DIO);
TM1637Display ledDisplay3(PIN_LED_DISPLAY_3_CLK, PIN_LED_DISPLAY_3_DIO);

bool relayStates[4];

bool DeMultiplex(int channel)
{
  digitalWrite(PIN_DECODER_S0, channel & 0b00000001);
  digitalWrite(PIN_DECODER_S1, channel & 0b00000010);
  digitalWrite(PIN_DECODER_S2, channel & 0b00000100);
  digitalWrite(PIN_DECODER_S3, channel & 0b00001000);

  delayMicroseconds(50); // Not sure if required.

  return analogRead(PIN_DECORDER_SIG) < 100 ? false : true;
}

void UpdateSynapticGenerator(bool trigger = false)
{
  static flasher flasherFlash(Pattern::Sin, 400, 255);
  flasherFlash.repeat(false);

  if (trigger)
  {
    flasherFlash.reset();
  }

  stripGenerator.fill(stripGenerator.Color(0, flasherFlash.getPwmValue(), 0), 0, stripGenerator.numPixels());
  stripGenerator.show();
}

void UpdateLedDisplays()
{
  static msTimer timer1(1000);
  static msTimer timer2(1000);
  static msTimer timer3(1000);
  static states previousState;
  static int value1, value2, value3;
  static int target1, target2, target3;

  int minDelay = state == stable ? 250 : state == warning ? 60 : state == critical ? 7 : 0;
  int maxDelay = state == stable ? 450 : state == warning ? 160 : state == critical ? 20 : 0;
  int minRange = state == stable ? 0 : state == warning ? 100 : state == critical ? 1000 : 0;
  int maxRange = state == stable ? 100 : state == warning ? 1000 : state == critical ? 10000 : 0;
  int spread = state == stable ? 1 : state == warning ? 9 : state == critical ? 21 : 0;

  if (previousState != state)
  {
    previousState = state;
    value1 = random(minRange, maxRange);
    value2 = random(minRange, maxRange);
    value3 = random(minRange, maxRange);
    timer1.resetDelay();
    timer2.resetDelay();
    timer3.resetDelay();
  }

  if (timer1.elapsed())
  {
    if (value1 > (target1 - spread) && value1 < (target1 + spread))
    {
      target1 = random(minRange, maxRange);
      timer1.setDelay(random(minDelay, maxDelay));
    }
    else
    {
      if (value1 > target1)
        value1 -= spread;
      if (value1 < target1)
        value1 += spread;
    }
    ledDisplay1.showNumberDecEx(value1);
  }

  if (timer2.elapsed())
  {
    if (value2 > (target2 - spread) && value2 < (target2 + spread))
    {
      target2 = random(minRange, maxRange);
      timer2.setDelay(random(minDelay, maxDelay));
    }
    else
    {
      if (value2 > target2)
        value2 -= spread;
      if (value2 < target2)
        value2 += spread;
    }
    ledDisplay2.showNumberDecEx(value2);
  }

  if (timer3.elapsed())
  {
    if (value3 > (target3 - spread) && value3 < (target3 + spread))
    {
      target3 = random(minRange, maxRange);
      timer3.setDelay(random(minDelay, maxDelay));
    }
    else
    {
      if (value3 > target3)
        value3 -= spread;
      if (value3 < target3)
        value3 += spread;
    }
    ledDisplay3.showNumberDecEx(value3);
  }
}

void UpdateRadiation()
{
  static flasher flashers[12];

  int delayRange = state == stable ? 3 : state == warning ? 2 : state == critical ? 1 : 0;
  Pattern pattern = state == stable ? Pattern::Sin : state == warning ? Pattern::OnOff : state == critical ? Pattern::Flash : Pattern::Flash;

  for (int i = 0; i < 12; i++)
  {
    int pwmValue = flashers[i].getPwmValue();

    if (pwmValue == 0)
    {
      flashers[i].setPattern(pattern);
      flashers[i].setDelay(random(500 * delayRange, 1000 * delayRange));
    }

    uint32_t color;
    if (i < 4)
      color = stripRadiation.Color(pwmValue, 0, 0);
    else if (i < 8)
      color = stripRadiation.Color(0, pwmValue, 0);
    else
      color = stripRadiation.Color(0, 0, pwmValue);

    stripRadiation.setPixelColor(i, color);
  }

  stripRadiation.show();
}

void UpdatePWMs()
{
  uint16_t pwms1[16];

  // Tachyon Sensormatic Grid : System in Terminal Flux
  int systemsInFlux = state == stable ? 1 : state == warning ? 2 : state == critical ? 3 : 0;
  int fluxDelay = state == stable ? 5000 : state == warning ? 3000 : state == critical ? 1000 : 0;

  static bool inFlux[4];
  static msTimer timerFlux(1000);
  if (timerFlux.elapsed())
  {
    timerFlux.setDelay(fluxDelay);
    RandomArrayFill(inFlux, systemsInFlux, sizeof(inFlux));
  }

  pwms1[0] = inFlux[0] ? maxPwmGenericLed : 0;
  pwms1[1] = inFlux[1] ? maxPwmGenericLed : 0;
  pwms1[2] = inFlux[2] ? maxPwmGenericLed : 0;
  pwms1[3] = inFlux[3] ? maxPwmGenericLed : 0;
  ///////////////////////////////////////////////////////

  // Photonic Lock 7
  int totalSubspaceValue = DeMultiplex(11) + DeMultiplex(12) + DeMultiplex(13) + DeMultiplex(14) + DeMultiplex(15);
  pwms1[7] = totalSubspaceValue % 2 == 0 ? maxPwmGreenLed : 0;

  // Bal 6
  static flasher flasherBal(Pattern::RandomReverseFlash, 2000, maxPwmGreenLed);
  if (state == stable)
    flasherBal.setDelay(2000);
  if (state == warning)
    flasherBal.setDelay(750);
  if (state == critical)
    flasherBal.setDelay(250);
  pwms1[6] = flasherBal.getPwmValue();

  // Delta Feedback 5
  static flasher flasherFeedback(Pattern::RandomFlash, 1000, maxPwmYellowLed);
  pwms1[5] = flasherFeedback.getPwmValue();

  // Router graphical indicators: Omega, imaginary, lambda : 10, 9, 8
  pwms1[10] = !DeMultiplex(8) ? maxPwmGenericLed : 0;
  pwms1[9] = !DeMultiplex(10) ? maxPwmGenericLed : 0;
  pwms1[8] = !DeMultiplex(9) ? maxPwmGenericLed : 0;

  // Router LED indicators: 15, 14, 13
  static flasher flasherRouter(Pattern::OnOff, 1000, maxPwmGenericLed);
  int routerDelay = state == stable ? 2000 : state == warning ? 1000 : state == critical ? 500 : 0;
  flasherRouter.setDelay(routerDelay);

  // Switches Vox, Stir, and RVS
  int switchVal = DeMultiplex(7) + DeMultiplex(4) + DeMultiplex(5);
  if (switchVal == 0)
    flasherRouter.setPattern(Pattern::OnOff);
  if (switchVal == 1)
    flasherRouter.setPattern(Pattern::Sin);
  if (switchVal == 2)
    flasherRouter.setPattern(Pattern::Flash);
  if (switchVal == 3)
    flasherRouter.setPattern(Pattern::RampUp);

  pwms1[15] = !DeMultiplex(8) ? flasherRouter.getPwmValue() : 0;
  pwms1[14] = !DeMultiplex(10) ? flasherRouter.getPwmValue() : 0;
  pwms1[13] = !DeMultiplex(9) ? flasherRouter.getPwmValue() : 0;

  // Post-Manafold 11
  static flasher flasherManafold(Pattern::Sin, 1000, maxPwmGenericLed);
  pwms1[11] = flasherManafold.getPwmValue();

  // Em. Pass: 12
  pwms1[12] = !DeMultiplex(6) ? maxPwmRedLed : 0;

  if (!IsPanelBootup(tachyonSensormaticGrid))
  {
    for (int i = 0; i < 4; i++)
    {
      pwms1[i] = 0;
    }
  }

  if (!IsPanelBootup(gndnPipelineRelay))
  {
    for (int i = 4; i < 16; i++)
    {
      pwms1[i] = 0;
    }
  }

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void ToggleRelay(int relay)
{
  if (relay > 3)
  {
    return;
  }

  relayStates[relay] = !relayStates[relay];
  digitalWrite(PIN_RELAY_LEFT_1, relayStates[0]);
  digitalWrite(PIN_RELAY_LEFT_2, relayStates[1]);
  digitalWrite(PIN_RELAY_RIGHT_1, relayStates[2]);
  digitalWrite(PIN_RELAY_RIGHT_2, relayStates[3]);
}

void ProcessSubspaceSwitches()
{
  // Subspace Synthesis, channels 11, 12, 13, 14, 15
  static msTimer timerDebounce(100);
  static bool debounceFlag;
  static int oldSubspaceValue;

  int subspaceValue = DeMultiplex(11) + DeMultiplex(12) + DeMultiplex(13) + DeMultiplex(14) + DeMultiplex(15);

  if (timerDebounce.elapsed())
    debounceFlag = false;

  if (oldSubspaceValue != subspaceValue && debounceFlag == false)
  {
    oldSubspaceValue = subspaceValue;

    timerDebounce.resetDelay();
    debounceFlag = true;

    ToggleRelay(random(0, 4));  
  }
}
void UpdateMultiplexIndicator()
{
  static msTimer timer(20);
  static byte wheelPos = 0;

  int delay = state == stable ? 20 : state == warning ? 15 : state == critical ? 10 : 0;
  timer.setDelay(delay);

  if (timer.elapsed())
  {
    wheelPos++;
  }

  int maxRand = state == stable ? 1000 : state == warning ? 90 : state == critical ? 35 : 0;
  static int hold = 0;
  if (state != stable && random(0, maxRand) == 0)
  {
    hold = 0;
  }

  if (++hold < 5)
  {
    stripManifold.fill(Color(255, 0, 0), 3, 3);
  }
  else
  {
    stripManifold.fill(Wheel(wheelPos), 3, 3);
  }

  stripManifold.show();
}


void UpdateManifoldIndicator()
{
  static flasher flasherManafold(Pattern::RandomReverseFlash, 500, 255);

  int delay = state == stable ? 1600 : state == warning ? 800 : state == critical ? 400 : 0;
  flasherManafold.setDelay(delay);

  if (state == stable)
  {
    stripManifold.fill(Color(0, 255, 0), 0, 3);
  }
  else
  {
    stripManifold.fill(Color(0, flasherManafold.getPwmValue(), 0), 0, 3);
  }
  stripManifold.show();
}

void UpdateGlyphIndicators()
{
  stripGlyphs.setPixelColor(0, DeMultiplex(9) ? 0 : Color(0, 0, 255));
  stripGlyphs.setPixelColor(1, DeMultiplex(10) ? 0 : Color(0, 255, 0));
  stripGlyphs.setPixelColor(2, DeMultiplex(8) ? 0 : Color(255, 0, 0));
  stripGlyphs.show();
}

void CheckToggleActivity()
{
  static int oldToggleSum;
  int toggleSum = 0;
  for (int i = 0; i < 16; i++)
  {
    toggleSum += DeMultiplex(i);
  }
  if (oldToggleSum != toggleSum)
  {
    oldToggleSum = toggleSum;
    activityFlag = true;
  }
}

void ShutdownPanelSensormaticGrid()
{
  stripRadiation.fill(0, 0, stripRadiation.numPixels());
  stripRadiation.show();

  ledDisplay1.clear();
  ledDisplay2.clear();
  ledDisplay3.clear();
}

void TurnOffAllRelays()
{
  digitalWrite(PIN_RELAY_LEFT_1, HIGH);
  digitalWrite(PIN_RELAY_LEFT_2, HIGH);
  digitalWrite(PIN_RELAY_RIGHT_1, HIGH);
  digitalWrite(PIN_RELAY_RIGHT_2, HIGH);
}

void ShutdownPanelGndnPipelineRelay()
{

  TurnOffAllRelays();

  stripGenerator.fill(0, 0, stripGenerator.numPixels());
  stripGenerator.show();
 
  stripManifold.fill(0, 0, stripManifold.numPixels());
  stripManifold.show();

  stripGlyphs.fill(0, 0, stripGlyphs.numPixels());
  stripGlyphs.show();
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(PIN_DECODER_S0, OUTPUT);
  pinMode(PIN_DECODER_S1, OUTPUT);
  pinMode(PIN_DECODER_S2, OUTPUT);
  pinMode(PIN_DECODER_S3, OUTPUT);
  pinMode(PIN_DECORDER_SIG, INPUT);

  pinMode(PIN_RELAY_LEFT_1, OUTPUT);
  pinMode(PIN_RELAY_LEFT_2, OUTPUT);
  pinMode(PIN_RELAY_RIGHT_1, OUTPUT);
  pinMode(PIN_RELAY_RIGHT_2, OUTPUT);

  TurnOffAllRelays();

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);

  stripManifold.begin(); 
  stripGenerator.begin();
  stripRadiation.begin();
  stripGlyphs.begin();
  stripGlyphs.show();

  ledDisplay1.setBrightness(2);
  ledDisplay2.setBrightness(2);
  ledDisplay3.setBrightness(2);
}

void loop()
{

  CheckControlData();

  UpdatePWMs();

  if (IsPanelBootup(tachyonSensormaticGrid))
  {
    UpdateRadiation();

    UpdateLedDisplays();
  }
  else
  {
    ShutdownPanelSensormaticGrid();
  }

  if (IsPanelBootup(gndnPipelineRelay))
  {
    if (performActivityFlag)
    {
      performActivityFlag = false;
      UpdateSynapticGenerator(true);
    }

    ProcessSubspaceSwitches();

    UpdateMultiplexIndicator();

    UpdateManifoldIndicator();

    UpdateGlyphIndicators();

    UpdateSynapticGenerator();

    CheckToggleActivity();
  }
  else
  {
    ShutdownPanelGndnPipelineRelay();
  }
}