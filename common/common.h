// Shared variables and methods for the collection of VX projects.
// Some methods are crude and intended only for the VX project's non-critical functionality.

#ifndef COMMON_H
#define COMMON_H

#define BAUD_RATE 57600

// When user interacts with a panel. (presses a button, toggles a switch, adjusts a pot).
bool activityFlag = false;

// Indicates to a panel to perform an activity as feedback to a user interacting with another panel.
bool performActivityFlag = false;

// Main system states. (determines flashing patterns and other behaviors).
enum states
{
	stable = 0,
	warning = 1,
	critical = 2,
	unknown = 3
} state;

// Indicates if a user is interacting with the system.
enum modes
{
	automaticActivity = 0,
	manualActivity = 1
} mode;

const int maxPwmGenericLed = 4095;
const int maxPwmRedLed = 1000;
const int maxPwmGreenLed = 4095;
const int maxPwmBlueLed = 1000;
const int maxPwmYellowLed = 4095;

// Shared method among the projects.
// triggerActivity should only be raised by the master panel.
void SendControlDataFromMaster(bool performActivity)
{
	byte checkSum = (byte)state + (byte)mode + 0 + (byte)performActivity;

	Serial.write((byte)state);
	Serial.write((byte)mode);
	Serial.write((byte)false); // activityFlag
	Serial.write((byte)performActivity);
	Serial.write((checkSum));
	Serial.write(13);
}

// Shared method among the projects.
// Master panel shall not echo the data.
void CheckControlData(bool echoFlag = true)
{
	static byte data[10];
	static int index;
	bool dataReadyFlag = false;

	while (Serial.available())
	{
		byte c = Serial.read();
		data[index] = c;

		if (++index > sizeof(data))
		{
			index = 0;
		}

		if (c == 13)
		{
			index = 0;
			dataReadyFlag = true;
		}
	}

	if (dataReadyFlag)
	{
		int checkSum = data[0] + data[1] + data[2] + data[3];

		if (checkSum != data[4])
		{
			return;
		}

		state = (states)data[0];
		mode = (modes)data[1];
		activityFlag = (bool)data[2] | activityFlag;
		performActivityFlag = (bool)data[3];

		if (echoFlag)
		{	
			byte checkSum = data[0] + data[1] + (data[2] | activityFlag) + data[3];
			
			Serial.write(data[0]);
			Serial.write(data[1]);
			Serial.write(data[2] | activityFlag);
			Serial.write(data[3]);
			Serial.write((checkSum));
			Serial.write(13);

			activityFlag = false;
		}
	}
}

// Returns true if x is in range [low..high], else false.
bool InRange(int x, int low, int high) 
{ 
    return ((x-high)*(x-low) <= 0); 
} 


// Incremment a value and rollover the results if the results are greater than the max value specified.
int RollOverValue(int value, int offset, int min, int max)
{
	int newValue = value + offset;

	if (newValue > max)
		newValue = min + newValue - max;

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

	if (r < 0)
		r = 0;
	if (g < 0)
		g = 0;
	if (b < 0)
		b = 0;

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

// Disperses a quantity of true values within a bool array.
bool RandomArrayFill(bool *array, int amountOfTrues, int size)
{
	if (amountOfTrues > size)
	{
		return false;
	}

	for (int i = 0; i < size; i++)
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