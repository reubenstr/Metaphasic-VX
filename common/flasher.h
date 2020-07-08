#ifndef FLASHER_H
#define FLASHER_H

#include <Arduino.h>
#include "msTimer.h"

enum class Pattern
{
    OnOff,
    Sin,
    RampUp, 
    Flash,
    RandomFlash
};

class flasher
{

private:
    Pattern _pattern;
    int _delay;
    int _maxPwm;
    int _pwmValue = 0;
    float _microsPerStep;
    unsigned long _oldMicros;

public:
    // Constructor.
    // Delay in milliseconds
    flasher(Pattern pattern, int delay, int maxPwm)
    {
        _pattern = pattern;
        _maxPwm = maxPwm;
        _delay = delay;        
    }

    inline int getPwmValue()
    {
        unsigned long curMicros = micros();

        if ((curMicros - _oldMicros) > _microsPerStep)
        {
            int stepsPassed = (float)(curMicros - _oldMicros) / _microsPerStep;

            if (_pattern == Pattern::RampUp)
            {
                _microsPerStep = 1.0 / (((float)_maxPwm/ (float)_delay) / 1000.0);
                _pwmValue += stepsPassed;

                if (_pwmValue > _maxPwm)
                {
                    _pwmValue = 0;
                }               
            } 
            else if (_pattern == Pattern::Sin)
            {
                _microsPerStep = 1.0 / ((180.0 / (float)_delay) / 1000.0);
                static byte sinIndex;
                sinIndex += stepsPassed;
                if (sinIndex > 180)
                {
                       sinIndex = 0; 
                } 
                 _pwmValue = _maxPwm * sin(radians(sinIndex));
            }            
            else if (_pattern == Pattern::OnOff)
            {
                  _microsPerStep = ((float)_delay / 2.0) * 1000.0; 
                  static bool toggle = false;
                  toggle = !toggle;
                  _pwmValue = toggle ? _maxPwm : 0;
            }
               else if (_pattern == Pattern::Flash)
            {
                 static bool toggleFlash = false;   
                if (toggleFlash)
                {
                    toggleFlash = false;
                    _microsPerStep = ((float)_delay / 10.0) * 1000.0 * 9;
                }
                else
                {
                    toggleFlash = true;
                    _microsPerStep = ((float)_delay / 10.0) * 1000.0;
                }                
                  _pwmValue = toggleFlash ? _maxPwm : 0;
            }
              else if (_pattern == Pattern::RandomFlash)
            {
                 static bool toggleFlash = false;   
                if (toggleFlash)
                {
                    toggleFlash = false;
                    _microsPerStep = ((float)random(_delay / 2, _delay * 1.5)) * 1000.0;
                }
                else
                {
                    toggleFlash = true;
                    _microsPerStep = 100 * 1000.0;
                }                
                  _pwmValue = toggleFlash ? _maxPwm : 0;
            }


             _oldMicros = curMicros;
        }

        return _pwmValue;
    }
};

#endif