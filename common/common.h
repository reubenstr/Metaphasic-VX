#ifndef COMMON_H
#define COMMON_H

enum states
{
  stable = 0,
  warning = 1,
  critical = 2
} state;

const int maxPwmGenericLed = 4095;
const int maxPwmRedLed = 1000;
const int maxPwmGreenLed = 4095;
const int maxPwmBlueLed = 1000;
const int maxPwmYellowLed = 4095;


#endif