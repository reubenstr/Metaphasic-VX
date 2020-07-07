#ifndef FLASHER_H
#define FLASHER_H

#include <Arduino.h>
#include "msTimer.h"

enum class Pattern
{
    Blink,
    Sin,
    RampUp
};

class flasher
{

private:
    Pattern _pattern;
    int _delay;
    int _maxPWM;
    int _pwmValue = 0;
    unsigned long _oldMicros;

public:
    // Constructor.
    // Delay in milliseconds
    flasher(Pattern pattern, int delay, int maxPwm)
    {
        _pattern = pattern;
        _delay = delay;
        _maxPWM = maxPwm;
    }

    inline int getPwmValue()
    {
        float microsPerStep = 1.0 / (((float)_maxPWM / (float)_delay) / 1000.0);

int stepsPassed;
        unsigned long curMicros = micros();

        if ((curMicros - _oldMicros) > microsPerStep)
        {
            stepsPassed = (float)(curMicros -_oldMicros) / microsPerStep;
         
            _pwmValue += stepsPassed;

            if (_pwmValue > _maxPWM)
            {
                _pwmValue = 0;
            }

            _oldMicros = curMicros;
        }

 //return stepsPassed; // TEMP

        return _pwmValue;
    }

    inline void setDelay(unsigned long delay)
    {
        _delay = delay;
    }
};

#endif