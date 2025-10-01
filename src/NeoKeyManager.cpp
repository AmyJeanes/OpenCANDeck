#include "NeoKeyManager.h"
#include "seesaw_neopixel.h"

bool NeoKeyManager::begin(uint8_t address)
{
    if (!_neokey.begin(address))
    {
        return false;
    }
    for (uint16_t i = 0; i < _neokey.pixels.numPixels(); i++)
    {
        _neokey.pixels.setPixelColor(i, seesaw_NeoPixel::Color(50, 0, 150));
        _neokey.pixels.show();
        delay(35);
    }
    _neokey.pixels.clear();
    _neokey.pixels.show();
    return true;
}

void NeoKeyManager::update()
{
    // Read buttons
    _currentButtons = _neokey.read();
    _changedMask = _currentButtons ^ _prevButtons;
    _justPressedMask = _changedMask & _currentButtons;
    _justReleasedMask = _changedMask & (~_currentButtons) & 0x0F;

    if (_changedMask)
    { // Only touch pixels when something changed
        for (uint8_t i = 0; i < _neokey.pixels.numPixels(); i++)
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
        _neokey.pixels.show();
    }
    _prevButtons = _currentButtons;
}
