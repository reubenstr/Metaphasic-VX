#include <Arduino.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <jled.h> // https://github.com/jandelgado/jled

#define PIN_POT_CORRECTION A0
#define PIN_STRIP_DCDC 7
#define PIN_STRIP_DISTRO 12
#define PIN_LED_LAMBDA_CORRECTION 10
#define PIN_LED_LOCK A1
#define PIN_LED_POWER_ON A2

#define TFT_CS  6
#define TFT_RST -1
#define TFT_DC 8
#define TFT_MOSI 9
#define TFT_SCLK   7

// Hardware SPI
// MOSI = pin 11
// SCLK = pin 13
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Adafruit_NeoPixel stripDCDC = Adafruit_NeoPixel(10, PIN_STRIP_DCDC, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel PIN_WS2812B_DISTRO = Adafruit_NeoPixel(3, PIN_STRIP_DISTRO, NEO_GRB + NEO_KHZ800);


auto ledLambdaCorrection = JLed(PIN_LED_LAMBDA_CORRECTION).Breathe(1500).Forever().DelayAfter(500);


enum states
{
  stable = 0,
  warning = 1,
  critical = 2
} state;


void updateLEDs()
{

  //digitalWrite(PIN_LED_LOCK, HIGH);

  ledLambdaCorrection.Update();

  static states previousState;
  if (state != previousState)
  {
    if (state == stable)
    {
      ledLambdaCorrection.Breathe(1500).Forever().DelayAfter(500);
    }
    else  if (state == warning)
    {
     ledLambdaCorrection.Breathe(1000).Forever().DelayAfter(350);
    }
    else if (state == critical)
    {
      ledLambdaCorrection.Breathe(500).Forever().DelayAfter(200);
    }
  }
  previousState = state;
}

void updateText()
{
  tft.setCursor(90, 6);
  tft.setTextWrap(true);
  tft.setTextSize(2);

  if (state == stable)
  {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.print("STABLE  ");
  }
  else  if (state == warning)
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


void updateIntensity()
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
  else  if (state == warning)
  {
    lowRange = xOffset  + width;
    highRange = xOffset + 2 * width;
    color = ST77XX_YELLOW;
  }
  else if (state == critical)
  {
    lowRange = xOffset  + 2 * width;
    highRange = xOffset + 3 * width;
    color = ST77XX_RED;
  }

  int intensity  = random(lowRange, highRange);

  /*
    uint16_t color = ST77XX_BLUE;
    if (map(intensity, xOffset, tft.width() - 2, 0, 3) == 0) color = ST77XX_GREEN;
    if (map(intensity, xOffset, tft.width() - 2, 0, 3) == 1) color = ST77XX_YELLOW;
    if (map(intensity, xOffset, tft.width() - 2, 0, 3) == 2) color = ST77XX_RED;
  */

  for (int yPos = 0; yPos < 12; yPos ++)
  {
    tft.drawLine(xOffset, yOffset + yPos, intensity, yOffset + yPos, color);
    tft.drawLine(intensity, yOffset + yPos, tft.width() - 2, yOffset + yPos, ST77XX_BLACK);
  }
}

void updateSin(int offset)
{
  static int oldOffset;
  int yOffset = 68;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              ;

  for (int x = 0; x < tft.width() - 1; x++)
  {
    int y = 25 * sin(x * oldOffset * 3.14 / 180);
    int y2 = 25 * sin(x * offset * 3.14 / 180);
    for (int i = 0; i < 1; i++)
    {
      tft.drawPixel(x, y + i  + yOffset, ST77XX_BLACK);
      tft.drawPixel(x, y2 + i  + yOffset, ST77XX_BLUE);
    }
  }

  oldOffset = offset;
}


void setup(void)
{

  Serial.begin(9600);

  pinMode(PIN_POT_CORRECTION, INPUT);
  pinMode(PIN_LED_LAMBDA_CORRECTION, OUTPUT);
  pinMode(PIN_LED_LOCK, OUTPUT);
  pinMode(PIN_LED_POWER_ON, OUTPUT);

  digitalWrite(PIN_LED_POWER_ON, HIGH);

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

  static unsigned long stateMillis;
  static unsigned long stateDelay;

  if ((stateMillis + stateDelay) < millis())
  {
    stateMillis = millis();
    stateDelay = 5000;
    state = (states)random(0, 3);
  }

  int offset = (map(analogRead(PIN_POT_CORRECTION), 0, 1024, 0, 10));

  updateSin(offset);

  updateIntensity();

  updateText();

  updateLEDs();


}