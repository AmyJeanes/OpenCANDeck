#include "NeoKeyManager.h"
#include "seesaw_neopixel.h"

bool NeoKeyManager::begin(uint8_t address)
{
    if (!_neokey.begin(address))
    {
        return false;
    }

    // Slide on
    for (uint16_t i = 0; i < _neokey.pixels.numPixels(); i++)
    {
        _neokey.pixels.setPixelColor(i, seesaw_NeoPixel::Color(50, 0, 150));
        _neokey.pixels.show();
        delay(75);
    }

    // Slide away
    for (uint16_t i = 0; i < _neokey.pixels.numPixels(); i++)
    {
        _neokey.pixels.setPixelColor(i, 0);
        _neokey.pixels.show();
        delay(75);
    }
    
    return true;
}

void NeoKeyManager::update()
{
    uint32_t now = millis();
    uint8_t newButtons = _neokey.read();

    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t mask = (1 << i);
        bool currentRawState = newButtons & mask;
        bool debouncedState = _currentButtons & mask;

        if (currentRawState != debouncedState)
        {
            // The button state has changed from its debounced state
            if (_lastChangeTime[i] == 0)
            {
                // This is the first time we see a change, start the timer
                _lastChangeTime[i] = now;
            }
            else if (now - _lastChangeTime[i] >= _debounceMs)
            {
                // The change has been stable for the debounce period.
                // Update the official debounced state.
                _currentButtons ^= mask; // Flip the bit
                _changedMask |= mask;
                if (currentRawState)
                {
                    _justPressedMask |= mask;
                }
                else
                {
                    _justReleasedMask |= mask;
                }
            }
        }
        else
        {
            // The state is stable, reset the timer
            _lastChangeTime[i] = 0;
        }
    }

    if (_changedMask)
    { // Only touch pixels when a debounced state change occurs
        for (uint8_t i = 0; i < 4; i++)
        {
            if (_changedMask & (1 << i))
            {
                if (_currentButtons & (1 << i))
                {
                    _neokey.pixels.setPixelColor(i, _pressedColor);
                }
                else
                {
                    _neokey.pixels.setPixelColor(i, 0);
                }
            }
        }
        _neokey.pixels.show();
        _changedMask = 0; // Clear the mask after updating pixels
    }
}

uint8_t NeoKeyManager::justPressed()
{
    uint8_t returnMask = _justPressedMask;
    _justPressedMask = 0;
    return returnMask;
}

uint8_t NeoKeyManager::justReleased()
{
    uint8_t returnMask = _justReleasedMask;
    _justReleasedMask = 0;
    return returnMask;
}
