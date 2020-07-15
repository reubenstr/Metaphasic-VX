#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include "common.h"            // Local libary.
#include "msTimer.h"           // Local libary.
#include "flasher.h"           // Local libary.

#define PIN_POT_CORRECTION A0
#define PIN_STRIP_DCDC 7
#define PIN_STRIP_DISTRIBUTION 5
#define PIN_LED_LAMBDA_CORRECTION 2
#define PIN_LED_LOCK A1
#define PIN_LED_POWER_ON A2
 
#define TFT_CS 6
#define TFT_RST -1
#define TFT_DC 8
// Hardware SPI : MOSI = pin 11, SCLK = pin 13
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Adafruit_NeoPixel stripDc = Adafruit_NeoPixel(10, PIN_STRIP_DCDC, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDistribution = Adafruit_NeoPixel(3, PIN_STRIP_DISTRIBUTION, NEO_GRB + NEO_KHZ800);

void UpdateStrips(int offset)
{
  uint32_t colorDc = state == stable ? Color(0, 225 - 25 * offset, 25 * offset) : state == warning ? Color(127, 127, 0) : state == critical ? Color(255, 0, 0) : 0;
  stripDc.fill(colorDc, 0, stripDc.numPixels());

  uint32_t colorDistribution = state == stable ? Color(0, 255, 0) : state == warning ? Color(127, 127, 0) : state == critical ? Color(255, 0, 0) : 0;
  stripDistribution.fill(colorDistribution, 0, stripDistribution.numPixels());

  stripDc.show();
  stripDistribution.show();
}

void UpdateLEDs()
{

  flasher flasherCorrection(Pattern::Sin, 1000, 255);

  int delayCorrection = state == stable ? 1000 : state == warning ? 750 : state == critical ? 500 : 0;
  flasherCorrection.setDelay(delayCorrection);
  analogWrite(PIN_LED_LAMBDA_CORRECTION, flasherCorrection.getPwmValue());
}

void UpdateLcdText()
{
  static states oldState;
  if (oldState != state)
  {
    oldState = state;

    tft.setCursor(90, 6);
    tft.setTextWrap(true);
    tft.setTextSize(2);

    if (state == stable)
    {
      tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
      tft.print("STABLE  ");
    }
    else if (state == warning)
    {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
      tft.print("WARNING ");
    }
    else if (state == critical)
    {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
      tft.print("CRITICAL");
    }
  }
}

void UpdateLcdIntensity()
{
  int xOffset = 124;
  int yOffset = 116;

  int width = (tft.width() - 2 - xOffset) / 3;
  int lowRange, highRange;
  uint16_t color;
  if (state == stable)
  {
    lowRange = xOffset;
    highRange = xOffset + width;
    color = ST77XX_GREEN;
  }
  else if (state == warning)
  {
    lowRange = xOffset + width;
    highRange = xOffset + 2 * width;
    color = ST77XX_YELLOW;
  }
  else if (state == critical)
  {
    lowRange = xOffset + 2 * width;
    highRange = xOffset + 3 * width;
    color = ST77XX_RED;
  }

  int intensity = random(lowRange, highRange);

  for (int yPos = 0; yPos < 12; yPos++)
  {
    tft.drawLine(xOffset, yOffset + yPos, intensity, yOffset + yPos, color);
    tft.drawLine(intensity, yOffset + yPos, tft.width() - 2, yOffset + yPos, ST77XX_BLACK);
  }
}

void UpdateLcdSin(int offset)
{
  static int oldOffset;
  int yOffset = 68;
  ;

  for (int x = 0; x < tft.width() - 1; x++)
  {
    int y = 25 * sin(x * oldOffset * 3.14 / 180);
    int y2 = 25 * sin(x * offset * 3.14 / 180);
    for (int i = 0; i < 1; i++)
    {
      tft.drawPixel(x, y + i + yOffset, ST77XX_BLACK);
      tft.drawPixel(x, y2 + i + yOffset, ST77XX_BLUE);
    }
  }

  oldOffset = offset;
}

void setup(void)
{

  Serial.begin(BAUD_RATE);

  pinMode(PIN_POT_CORRECTION, INPUT);
  pinMode(PIN_LED_LAMBDA_CORRECTION, OUTPUT);
  pinMode(PIN_LED_LOCK, OUTPUT);
  pinMode(PIN_LED_POWER_ON, OUTPUT);

  digitalWrite(PIN_LED_POWER_ON, HIGH);

  stripDc.begin();
  stripDistribution.begin();

  tft.init(135, 240);

  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(5, 6);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.setTextSize(2);
  tft.print("STATUS: ");

  tft.setCursor(5, 115);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.setTextSize(2);
  tft.print("INTENSITY:");

  // Draw border.
  uint16_t color = ST77XX_WHITE;
  tft.drawLine(0, 0, 0, tft.height() - 1, color);
  tft.drawLine(0, tft.height() - 1, tft.width() - 1, tft.height() - 1, color);
  tft.drawLine(tft.width() - 1, tft.height() - 1, tft.width() - 1, 0, color);
  tft.drawLine(tft.width() - 1, 0, 0, 0, color);
  // Draw misc. features.
  tft.drawLine(0, 25, tft.width() - 1, 25, color);
  tft.drawLine(0, 109, tft.width() - 1, 109, color);
}

void loop()
{
  // State controller for the entire VX system.
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

  static msTimer timerSendData(1000);
  if (timerSendData.elapsed())
  {
    SendControlData(activityFlag);
    activityFlag = false;
  }

  CheckControlData(false);

  int offset = (map(analogRead(PIN_POT_CORRECTION), 0, 1024, 0, 10));

  UpdateLcdSin(offset);

  UpdateLcdIntensity();

   UpdateLcdText();

  UpdateLEDs();

  UpdateStrips(offset);

 
}