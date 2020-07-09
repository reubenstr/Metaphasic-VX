#include <Arduino.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_STRIP_ISOLINEAR_MANAFOLD A1
#define PIN_STRIP_DYNAMIC_MULTIPLEX 13
#define PIN_STRIP_SYNAPTIC_GENERATOR A0
#define PIN_STRIP_RADIATION 12

#define PIN_RELAY_LEFT_1 A3
#define PIN_RELAY_LEFT_2 A6
//#define PIN_RELAY_RIGHT_1 A2
#define PIN_RELAY_RIGHT_2 A7

#define PIN_DECODER_S0 2
#define PIN_DECODER_S1 3
#define PIN_DECODER_S2 4
#define PIN_DECODER_S3 5
#define PIN_DECORDER_SIG A2

#define PIN_LED_DISPLAY_1_CLK 6
#define PIN_LED_DISPLAY_2_CLK 8
#define PIN_LED_DISPLAY_3_CLK 10
#define PIN_LED_DISPLAY_1_DIO 7
#define PIN_LED_DISPLAY_2_DIO 9
#define PIN_LED_DISPLAY_3_DIO 11

Adafruit_NeoPixel stripManafold = Adafruit_NeoPixel(3, PIN_STRIP_ISOLINEAR_MANAFOLD, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripMuliplex = Adafruit_NeoPixel(3, PIN_STRIP_DYNAMIC_MULTIPLEX, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripGenerator = Adafruit_NeoPixel(3, PIN_STRIP_SYNAPTIC_GENERATOR, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel stripRadiation = Adafruit_NeoPixel(12, PIN_STRIP_RADIATION, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;

TM1637Display ledDisplay1(PIN_LED_DISPLAY_1_CLK, PIN_LED_DISPLAY_1_DIO);
TM1637Display ledDisplay2(PIN_LED_DISPLAY_2_CLK, PIN_LED_DISPLAY_2_DIO);
TM1637Display ledDisplay3(PIN_LED_DISPLAY_3_CLK, PIN_LED_DISPLAY_3_DIO);

void UpdateLedDisplays()
{
  ledDisplay1.showNumberDecEx(1234, 1);
  ledDisplay2.showNumberDecEx(4567, 2);
  ledDisplay3.showNumberDecEx(8901, 3);
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

void SetPWMs()
{

  uint16_t pwms1[16];

  for (int i = 0; i < 16; i++)
  {
    pwms1[i] = 4095;
  }

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

bool DeMultiplex(int channel)
{
  digitalWrite(PIN_DECODER_S0, channel & 0b00000001);
  digitalWrite(PIN_DECODER_S1, channel & 0b00000010);
  digitalWrite(PIN_DECODER_S2, channel & 0b00000100);
  digitalWrite(PIN_DECODER_S3, channel & 0b00001000);

  return digitalRead(PIN_DECORDER_SIG);
}

void ProcessSwitches()
{

  // Subspace Synthesis, channels 11, 12, 13, 14, 15

  // Em. Pass, channel 6

  // Vox, channel 7

  // Stir O2, channel 4

  // 88-n RVS, channel 5

  // Omicron Relay (omega, imaginary, lambda), channels 8, 10, 9
}

void setup()
{

  delay(500);

  Serial.begin(57600);

  pinMode(PIN_DECODER_S0, OUTPUT);
  pinMode(PIN_DECODER_S1, OUTPUT);
  pinMode(PIN_DECODER_S2, OUTPUT);
  pinMode(PIN_DECODER_S3, OUTPUT);
  pinMode(PIN_DECORDER_SIG, INPUT);

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

  SetPWMs();

  ProcessSwitches();

  UpdateRadiation();

  UpdateLedDisplays();

  stripManafold.fill(stripManafold.Color(255, 0, 0), 0, stripManafold.numPixels());
  stripMuliplex.fill(stripMuliplex.Color(0, 255, 0), 0, stripMuliplex.numPixels());
  stripGenerator.fill(stripGenerator.Color(0, 0, 255), 0, stripGenerator.numPixels());

  stripManafold.show();
  stripMuliplex.show();
  stripGenerator.show();
}