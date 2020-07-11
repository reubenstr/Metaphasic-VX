// Shared variables and methods for the collection of VX projects.
// Note: some methods are crude and intended for non-critical functionality.

#ifndef COMMON_H
#define COMMON_H

#define BAUD_RATE 57600

bool activityFlag = false;

enum states
{
  stable = 0,
  warning = 1,
  critical = 2
} state;

enum modes
{
	passive = 0,
	active = 1
} mode;

const int maxPwmGenericLed = 4095;
const int maxPwmRedLed = 1000;
const int maxPwmGreenLed = 4095;
const int maxPwmBlueLed = 1000;
const int maxPwmYellowLed = 4095;


int RollOverValue(int value, int offset, int min, int max)
{
	int newValue = value + offset;
	
	if (newValue > max) newValue = min + newValue - max;

	return newValue;	
}

// Pack color data into 32 bit unsigned int (copied from Neopixel library).
uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint32_t)((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}


// Fade color by specified amount.
uint32_t Fade(uint32_t color, int amt)
{
	signed int r, g, b;
	
	r = (color & 0x00FF0000) >> 16;
	g = (color & 0x0000FF00) >> 8;
	b = color & 0x000000FF;
	
	r -= amt;
	g -= amt;
	b -= amt;
	
	
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;
		
  return Color(r, g, b);
}

// Input a value 0 to 255 to get a color value (of a pseudo-rainbow).
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


// Count number of true values in bool array.
int CountTruesInArray(bool *arr, int size)
{
  int count = 0;
  
  for (int i = 0; i < size; i++)
  {
    if (arr[i] == true)
    {
      count++;
    }
  }
  return count;
}

// Disperses a quantity of true values within a bool array with no overlap.
bool RandomArrayFill(bool* array, int amountOfTrues, int size)
{			
	if (amountOfTrues > size)
	{
		return false;
	}
	
	for(int i = 0; i < size; i++)
	{
		array[i] = false;
	}	
	
	while (CountTruesInArray(array, size) != amountOfTrues)
	{
		array[random(0, size)] = true;
	}
	
	return true;
}

#endif