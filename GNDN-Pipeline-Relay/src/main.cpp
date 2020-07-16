#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_STRIP_ISOLINEAR_MANAFOLD 13
#define PIN_STRIP_DYNAMIC_MULTIPLEX A1
#define PIN_STRIP_SYNAPTIC_GENERATOR A0
#define PIN_STRIP_RADIATION 12

#define PIN_RELAY_LEFT_1 A3
#define PIN_RELAY_LEFT_2 A2
#define PIN_RELAY_RIGHT_1 10
#define PIN_RELAY_RIGHT_2 8

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

Adafruit_NeoPixel stripManafold = Adafruit_NeoPixel(3, PIN_STRIP_ISOLINEAR_MANAFOLD, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripMuliplex = Adafruit_NeoPixel(3, PIN_STRIP_DYNAMIC_MULTIPLEX, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripGenerator = Adafruit_NeoPixel(3, PIN_STRIP_SYNAPTIC_GENERATOR, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripRadiation = Adafruit_NeoPixel(12, PIN_STRIP_RADIATION, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;

TM1637Display ledDisplay1(PIN_LED_DISPLAY_1_CLK, PIN_LED_DISPLAY_1_DIO);
TM1637Display ledDisplay2(PIN_LED_DISPLAY_2_CLK, PIN_LED_DISPLAY_2_DIO);
TM1637Display ledDisplay3(PIN_LED_DISPLAY_3_CLK, PIN_LED_DISPLAY_3_DIO);

int temp;

bool DeMultiplex(int channel)
{
  digitalWrite(PIN_DECODER_S0, channel & 0b00000001);
  digitalWrite(PIN_DECODER_S1, channel & 0b00000010);
  digitalWrite(PIN_DECODER_S2, channel & 0b00000100);
  digitalWrite(PIN_DECODER_S3, channel & 0b00001000);

  return analogRead(PIN_DECORDER_SIG) < 100 ? false : true;
}

void UpdateSynapticGenerator(bool trigger)
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

  int minDelay = state == stable ? 100 : state == warning ? 40 : state == critical ? 5 : 0;
  int maxDelay = state == stable ? 300 : state == warning ? 120 : state == critical ? 15 : 0;
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

  // System in Terminal Flux
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

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void ToggleRelay(int relay)
{
  if (relay == 0)
    digitalWrite(PIN_RELAY_LEFT_1, !digitalRead(PIN_RELAY_LEFT_1));
  else if (relay == 1)
    digitalWrite(PIN_RELAY_LEFT_2, !digitalRead(PIN_RELAY_LEFT_2));
  else if (relay == 2)
    digitalWrite(PIN_RELAY_RIGHT_1, !digitalRead(PIN_RELAY_RIGHT_1));
  else if (relay == 3)
    digitalWrite(PIN_RELAY_RIGHT_2, !digitalRead(PIN_RELAY_RIGHT_2));
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
    UpdateSynapticGenerator(true);
  }
}

void UpdateStripIndicators()
{
  static flasher flasherManafold(Pattern::RandomReverseFlash, 500, 255);
  static flasher flasherMuliplex(Pattern::RandomReverseFlash, 500, 255);

  int delay = state == stable ? 1600 : state == warning ? 800 : state == critical ? 400 : 0;
  flasherManafold.setDelay(delay);
  flasherMuliplex.setDelay(delay);

  if (state == stable)
  {
    stripManafold.fill(stripManafold.Color(0, 255, 0), 0, stripManafold.numPixels());
    stripMuliplex.fill(stripMuliplex.Color(0, 255, 0), 0, stripMuliplex.numPixels());
  }
  else
  {
    stripManafold.fill(stripManafold.Color(flasherManafold.getPwmValue(), 0, 0), 0, stripManafold.numPixels());
    stripMuliplex.fill(stripMuliplex.Color(flasherMuliplex.getPwmValue(), 0, 0), 0, stripMuliplex.numPixels());
  }

  stripManafold.show();
  stripMuliplex.show();
}

void UpdateRelayToggle()
{
  // Toggle relays when mode is active.
  static msTimer timerRelays(2000);
  int delayRelays = state == stable ? 1500 : state == warning ? 700 : state == critical ? 350 : 0;

  if (mode == automaticActivity)
  {
    if (timerRelays.elapsed())
    {
      timerRelays.setDelay(delayRelays + random(0, delayRelays));

      ToggleRelay(random(0, 3));
      UpdateSynapticGenerator(true);
    }
  }
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

void setup()
{
  delay(500);

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

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x40);
  pwmController1.setPWMFrequency(1500);

  stripManafold.begin();
  stripMuliplex.begin();
  stripGenerator.begin();
  stripRadiation.begin();

  ledDisplay1.setBrightness(2);
  ledDisplay2.setBrightness(2);
  ledDisplay3.setBrightness(2);
}

void loop()
{

  CheckControlData();

  if (performActivityFlag)
  {
    performActivityFlag = false;

    ToggleRelay(random(0, 3));
    UpdateSynapticGenerator(true);
  }

  UpdatePWMs();

  ProcessSubspaceSwitches();

  UpdateRadiation();

  UpdateLedDisplays();

  UpdateStripIndicators();

  UpdateSynapticGenerator(false);

  // UpdateRelayToggle(); // TEMP

  CheckToggleActivity();
}