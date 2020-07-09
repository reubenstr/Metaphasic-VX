#include <Arduino.h>
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include "LedControl.h"        // https://platformio.org/lib/show/914/LedControl
#include <JC_Button.h>         // https://github.com/JChristensen/JC_Button
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include <common.h>            // Local libary.
#include <msTimer.h>           // Local libary.
#include <flasher.h>         // Local libary.

#define PIN_MATRIX_DATAIN 4
#define PIN_MATRIX_LOAD 3
#define PIN_MATRIX_CLK 2

#define PIN_STRIP_SENTIENCE_DETECTED 5
#define PIN_BUTTON_ABORT 6
#define PIN_BUTTON_SKYNET A3
#define PIN_BUTTON_LCARS A2
#define PIN_BUTTON_KITT A0
#define PIN_BUTTON_HAL A1
#define PIN_TOGGLE_SUPPRESSION A6

// data, clk, load, number of matrix
LedControl lc = LedControl(PIN_MATRIX_DATAIN, PIN_MATRIX_CLK, PIN_MATRIX_LOAD, 3);

Adafruit_NeoPixel stripSentienceDetected = Adafruit_NeoPixel(3, PIN_STRIP_SENTIENCE_DETECTED, NEO_GRB + NEO_KHZ800);

PCA9685 pwmController1;

Button buttonAbort(PIN_BUTTON_ABORT);
Button buttonSkynet(PIN_BUTTON_SKYNET);
Button buttonLcars(PIN_BUTTON_LCARS);
Button buttonKitt(PIN_BUTTON_KITT);
Button buttonHal(PIN_BUTTON_HAL);

bool cpuMatrix[8][8];

byte hal0[8] = {B00000000, B00000000, B00000000, B00011000, B00011000, B00000000, B00000000, B00000000};
byte hal1[8] = {B00000000, B00000000, B00111100, B00111100, B00111100, B00111100, B00000000, B00000000};
byte hal2[8] = {B00000000, B01111110, B01111110, B01111110, B01111110, B01111110, B01111110, B00000000};
byte lcarsVals[8] = {B10000000, B11000000, B11100000, B11110000, B11111000, B11111100, B11111110, B11111111};

struct RainDrop
{
  byte col;
  byte row;
  int speed;
  unsigned long millis;
};

struct KitLed
{
  signed int col;
  signed int row;
  int speed;
  bool direction;
  unsigned long millis;
} kitLeds[8];

enum aiStates
{
  skynet,
  lcars,
  kitt,
  hal
} aiState;


const int numErrors = 4;
bool errors[numErrors];

bool flipFlop;
bool sentienceDetected;
bool abortFlag;

void MemoryBank()
{
  static msTimer timer(1000);
  if (timer.elapsed())
  {
    for (int row = 0; row < 8; row++)
    {
      for (int col = 0; col < 8; col++)
      {
        if (random(2) == 0) // Hides refresh pattern.
        {
          if (random(3) == 0)
            lc.setLed(0, row, col, true);
          else
            lc.setLed(0, row, col, false);
        }
      }
    }
  }
}

void Skynet()
{
  static RainDrop drops[10];

  for (int drop = 0; drop < 6; drop++)
  {
    if ((drops[drop].millis + drops[drop].speed) < millis())
    {
      drops[drop].millis = millis();

      lc.setLed(1, drops[drop].row, drops[drop].col, false);
      lc.setLed(2, drops[drop].row, drops[drop].col, flipFlop ? true : false);

      drops[drop].row--;

      if (drops[drop].row == 255) // Backwards roll-over.
      {
        drops[drop].row = 8; // random(4);
        drops[drop].col = random(8);
        drops[drop].speed = 50 + random(400);
      }

      lc.setLed(1, drops[drop].row, drops[drop].col, true);
      lc.setLed(2, drops[drop].row, drops[drop].col, flipFlop ? false : true);
    }
  }
}

void Lcars()
{
  static msTimer timer(500);
  if (timer.elapsed())
  {
    int col = random(0, 8);
    int val = random(0, 8);
    lc.setColumn(1, col, lcarsVals[val]);
    lc.setColumn(2, col, flipFlop ? ~lcarsVals[val] : lcarsVals[val]);
  }
}

void Hal()
{
  static msTimer timer(500);
  if (timer.elapsed())
  {
    static byte step;

    if (++step > 3)
    {
      step = 0;
    }

    for (int i = 0; i < 8; i++)
    {
      if (step == 0)
      {
        lc.setRow(1, i, hal0[i]);
        lc.setRow(2, i, flipFlop ? ~hal0[i] : hal0[i]);
      }
      else if (step == 1)
      {
        lc.setRow(1, i, hal1[i]);
        lc.setRow(2, i, flipFlop ? ~hal1[i] : hal1[i]);
      }
      else if (step == 2)
      {
        lc.setRow(1, i, hal2[i]);
        lc.setRow(2, i, flipFlop ? ~hal2[i] : hal2[i]);
      }
    }
  }
}

void Kitt()
{
  for (int i = 0; i < 8; i++)
  {
    if ((kitLeds[i].millis + kitLeds[i].speed) < millis())
    {

      kitLeds[i].millis = millis();

      lc.setLed(1, i, kitLeds[i].col, false);
      lc.setLed(2, i, kitLeds[i].col, flipFlop ? true : false);

      if (kitLeds[i].direction)
      {
        kitLeds[i].col++;
      }
      else
      {
        kitLeds[i].col--;
      }

      kitLeds[i].speed = 100; // TEMP

      if (kitLeds[i].col == -1)
      {
        kitLeds[i].col = 0;
        kitLeds[i].direction = !kitLeds[i].direction;
        //kitLeds[i].speed = 50 + random(400);
      }

      if (kitLeds[i].col == 8)
      {
        kitLeds[i].col = 7;
        kitLeds[i].direction = !kitLeds[i].direction;
        //kitLeds[i].speed = 50 + random(400);
      }
      lc.setLed(1, i, kitLeds[i].col, true);
      lc.setLed(2, i, kitLeds[i].col, flipFlop ? false : true);
    }
  }
}

void UpdatePWMs()
{
  uint16_t pwms1[16];

  // Error states/messages.
  for (int i = 0; i < 4; i++)
  {
    if (errors[i])
    {
      pwms1[i] = maxPwmRedLed;
    }
    else
    {
      pwms1[i] = 0;
    }
  }

  pwms1[11] = aiState == skynet ? maxPwmGenericLed : 0;
  pwms1[10] = aiState == lcars ? maxPwmGenericLed : 0;
  pwms1[9] = aiState == kitt ? maxPwmGenericLed : 0;
  pwms1[8] = aiState == hal ? maxPwmGenericLed : 0;

  // FlipFlop
  pwms1[4] = flipFlop ? maxPwmBlueLed : 0;

  // Transfer
  static flasher flasherTransfer(Pattern::RandomFlash, 500, maxPwmGreenLed);
  pwms1[5] = flasherTransfer.getPwmValue();

  // ECC
  static flasher flasherEcc(Pattern::RandomFlash, 2000, maxPwmYellowLed);
  pwms1[6] = flasherEcc.getPwmValue();

  pwmController1.setChannelsPWM(0, 16, pwms1);
}

void SetStrips()
{
  if (sentienceDetected)
  {
    static flasher flasherStrip(Pattern::OnOff, 1000, 255);
    int colorValue = flasherStrip.getPwmValue();
    stripSentienceDetected.fill(stripSentienceDetected.Color(colorValue, 0, 0), 0, stripSentienceDetected.numPixels());
  }
  else
  {
    stripSentienceDetected.fill(stripSentienceDetected.Color(0, 0, 0), 0, stripSentienceDetected.numPixels());
  }
  stripSentienceDetected.show();
}

void CheckButtons()
{
  buttonAbort.read();
  buttonSkynet.read();
  buttonLcars.read();
  buttonKitt.read();
  buttonHal.read();

  if (buttonAbort.wasPressed())
  {
    if (sentienceDetected)
    {
      abortFlag = true;
    }
  }

  if (!sentienceDetected)
  {
    if (buttonSkynet.wasPressed())
    {
      aiState = skynet;
    }

    if (buttonLcars.wasPressed())
    {
      aiState = lcars;
    }

    if (buttonKitt.wasPressed())
    {
      aiState = kitt;
    }

    if (buttonHal.wasPressed())
    {
      aiState = hal;
    }
  }
}



void ProcessErrors()
{
  static msTimer errorTimer(1000);
  if (errorTimer.elapsed())
  {

    for (int i = 0; i < numErrors; i++)
    {
      errors[i] = false;
    }

    int errorCount = CountTruesInArray(errors, numErrors);
    int targetErrorCount = (int)state + 1;

    while (errorCount != targetErrorCount)
    {
      if (errorCount > targetErrorCount)
      {
        errors[random(0, numErrors + 1)] = false;
      }
      if (errorCount < targetErrorCount)
      {
        errors[random(0, numErrors + 1)] = true;
      }
      errorCount = CountTruesInArray(errors, numErrors);
    }
  }
}

void AbortSequence()
{
  sentienceDetected = false;
  SetStrips();

  pwmController1.setAllChannelsPWM(0);

  byte empty = 0;
  for (int i = 0; i < 8; i++)
  {
    lc.setRow(0, 7 - i, empty);
    lc.setRow(1, 7 - i, empty);
    lc.setRow(2, 7 - i, empty);
    delay(250);
  }

  for (int i = 0; i < 8; i++)
  {
    int pwmValue = ((i % 2) == 0) ? 0 : 1500;
    pwmController1.setAllChannelsPWM(pwmValue);
    delay(250);
  }
}

void setup()
{
  delay(500);
  Serial.begin(9600);

  buttonAbort.begin();
  buttonSkynet.begin();
  buttonLcars.begin();
  buttonKitt.begin();
  buttonHal.begin();

  Wire.begin();
  pwmController1.resetDevices();
  pwmController1.init(0x00);
  pwmController1.setPWMFrequency(1500);

  lc.shutdown(0, false);
  lc.setIntensity(0, 2);
  lc.clearDisplay(0);

  lc.shutdown(1, false);
  lc.setIntensity(1, 2);
  lc.clearDisplay(1);

  lc.shutdown(2, false);
  lc.setIntensity(2, 2);
  lc.clearDisplay(2);

  for (int i = 0; i < 8; i++)
  {
    kitLeds[i].col = i;
  }

  stripSentienceDetected.begin();
}

void loop()
{
  //state = critical;         // TEMP
  //sentienceDetected = true; // TEMP
  //aiState = lcars;          // TEMP

  if (abortFlag)
  {
    abortFlag = false;
    AbortSequence();
  }

  static msTimer flipFlopTimer(2500);
  if (flipFlopTimer.elapsed())
  {
    flipFlop = !flipFlop;
  }

  SetStrips();

  UpdatePWMs();

  CheckButtons();

  ProcessErrors();

  MemoryBank();

  if (aiState == skynet)
  {
    Skynet();
  }
  else if (aiState == lcars)
  {
    Lcars();
  }
  else if (aiState == kitt)
  {
    Kitt();
  }
  else if (aiState == hal)
  {
    Hal();
  }
}
