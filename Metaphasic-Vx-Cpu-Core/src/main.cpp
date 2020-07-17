#include <Arduino.h>
#include "PCA9685.h"           // https://github.com/NachtRaveVL/PCA9685-Arduino
#include "LedControl.h"        // https://platformio.org/lib/show/914/LedControl
#include <JC_Button.h>         // https://github.com/JChristensen/JC_Button
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include <common.h>            // Local libary.
#include <msTimer.h>           // Local libary.
#include <flasher.h>           // Local libary.

#define PIN_MATRIX_DATAIN 4
#define PIN_MATRIX_LOAD 3
#define PIN_MATRIX_CLK 2

#define PIN_STRIP_SENTIENCE_DETECTED 5
#define PIN_BUTTON_ABORT 6
#define PIN_BUTTON_SKYNET A3
#define PIN_BUTTON_LCARS A2
#define PIN_BUTTON_KITT A0
#define PIN_BUTTON_HAL A1
#define PIN_TOGGLE_SUPPRESSION 12

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

  pwms1[12] = 0;
  pwms1[11] = aiState == skynet ? maxPwmGenericLed : 0;
  pwms1[10] = aiState == lcars ? maxPwmGenericLed : 0;
  pwms1[9] = aiState == kitt ? maxPwmGenericLed : 0;
  pwms1[8] = aiState == hal ? maxPwmGenericLed : 0;

  // TEMP
  pwms1[9] = mode == manualActivity ? maxPwmGenericLed : 0;

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

  if (buttonSkynet.wasPressed())
  {
    if (!sentienceDetected)
    {
      aiState = skynet;
      activityFlag = true;
    }
  }

  if (buttonLcars.wasPressed())
  {
    if (!sentienceDetected)
    {
      aiState = lcars;
      activityFlag = true;
    }
  }

  if (buttonKitt.wasPressed())
  {
    if (!sentienceDetected)
    {
      aiState = kitt;
      activityFlag = true;
    }
  }

  if (buttonHal.wasPressed())
  {
    if (!sentienceDetected)
    {
      aiState = hal;
      activityFlag = true;
    }
  }
}

void ProcessErrors()
{
  static msTimer errorTimer(1000);
  int delay = state == stable ? 2500 : state == warning ? 1500 : state == critical ? 750 : 0;

  if (errorTimer.elapsed())
  {
    errorTimer.setDelay(random(delay, delay * 2));
    int quantity = state == stable ? 1 : state == warning ? 2 : state == critical ? 3 : 0;
    RandomArrayFill(errors, quantity, sizeof(errors));
  }
}

void UpdateSentienceIndicator()
{
  if (sentienceDetected)
  {
    static flasher flasherStrip(Pattern::OnOff, 1000, 255);
    stripSentienceDetected.fill(stripSentienceDetected.Color(flasherStrip.getPwmValue(), 0, 0), 0, stripSentienceDetected.numPixels());
  }
  else
  {
    stripSentienceDetected.fill(stripSentienceDetected.Color(0, 0, 0), 0, stripSentienceDetected.numPixels());
  }
  stripSentienceDetected.show();
}

void AbortSequence()
{
  sentienceDetected = false;
  UpdateSentienceIndicator();

  uint16_t pwms1[16];
  for (int i = 0; i < 16; i++)
  {
    pwms1[i] = 0;
  }

  byte empty = 0;
  for (int i = 0; i < 8; i++)
  {
    lc.setRow(0, 7 - i, empty);
    lc.setRow(1, 7 - i, empty);
    lc.setRow(2, 7 - i, empty);
    pwms1[12] = ~pwms1[12];
    pwmController1.setChannelsPWM(0, 16, pwms1);
    delay(250);
  }

  for (int i = 0; i < 8; i++)
  {
    int pwmValue = ((i % 2) == 0) ? 0 : 1500; 
    for (int i = 0; i < 16; i++)
    {
      pwms1[i] = pwmValue;
    }
    pwms1[12] = ((i % 2) == 1) ? 0 : 1500;
    pwmController1.setChannelsPWM(0, 16, pwms1);
    delay(250);
  }
}

void UpdateSentience()
{
  static msTimer timerSentience(20000);
  static msTimer timerSentienceComplete(8000);
  if (timerSentience.elapsed())
  {
    sentienceDetected = true;
    timerSentienceComplete.resetDelay();
  }

  if (timerSentienceComplete.elapsed())
  {
    sentienceDetected = false;
  }

  if (abortFlag)
  {
    abortFlag = false;
    AbortSequence();
    timerSentience.resetDelay();
  }
}

void CheckToggle()
{
  // TODO: might need debounce.
  static bool oldToggleSuppression;
  if (oldToggleSuppression != digitalRead(PIN_TOGGLE_SUPPRESSION))
  {
    oldToggleSuppression = digitalRead(PIN_TOGGLE_SUPPRESSION);
    activityFlag = true;
  }
}

void setup()
{
  delay(500);
  Serial.begin(BAUD_RATE);

  pinMode(PIN_TOGGLE_SUPPRESSION, INPUT);
  digitalWrite(PIN_TOGGLE_SUPPRESSION, HIGH);

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

  aiState = lcars; // TEMP
}

void loop()
{

  // State controller for the entire VX system.
  /*
  static msTimer timerState(5000);
  if (timerState.elapsed())
  {
    //state = (states)random(0, 3);
    static int s = 0;
    if (++s > 2)
    {
      s = 0;
    }

    state = (states)s;
  }
  */

  // TEMP
  if (aiState == skynet)
  {
    state = critical;
  }
  if (aiState == lcars)
  {
    state = stable;
  }
  if (aiState == hal)
  {
    state = warning;
  }
  // TEMP


  static msTimer timerActivityTimeout(8000);
  static msTimer timerSendData(100);

  CheckControlData(false);

  if (timerSendData.elapsed())
  {
    if (activityFlag)
    {
      activityFlag = false;
      mode = manualActivity;
      timerActivityTimeout.resetDelay();  

        SendControlDataFromMaster(true);         
    }
    else
    {
        SendControlDataFromMaster(false);
    }    
  }

  if (timerActivityTimeout.elapsed())
  {
    mode = automaticActivity;
  }

  

  static msTimer flipFlopTimer(2500);
  if (flipFlopTimer.elapsed())
  {
    flipFlop = !flipFlop;
  }

  // UpdateSentience(); // TEMP

  UpdateSentienceIndicator();

  CheckToggle();

  UpdatePWMs();

  CheckButtons();

  ProcessErrors();

  MemoryBank();

  if (aiState == skynet)
    Skynet();
  else if (aiState == lcars)
    Lcars();
  else if (aiState == kitt)
    Kitt();
  else if (aiState == hal)
    Hal();
}
