#ifndef COMMON_H
#define COMMON_H

#define BAUD_RATE 57600


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

// Disperses a quantity of true values within an bool array.
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