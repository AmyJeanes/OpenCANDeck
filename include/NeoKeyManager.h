// Encapsulates interaction with Adafruit NeoKey 1x4 module.
#pragma once

#include <Arduino.h>
#include "Adafruit_NeoKey_1x4.h"

class NeoKeyManager {
public:
    bool begin(uint8_t address);
    void update();
    void setPressedColor(uint32_t color) { _pressedColor = color; }
    uint8_t buttons() const { return _currentButtons; }
    uint8_t buttonsChanged() const { return _changedMask; }
    uint8_t justPressed() const { return _justPressedMask; }
    uint8_t justReleased() const { return _justReleasedMask; }

private:
    Adafruit_NeoKey_1x4 _neokey;
    uint8_t _prevButtons = 0;
    uint8_t _currentButtons = 0;
    uint8_t _changedMask = 0;
    uint8_t _justPressedMask = 0;
    uint8_t _justReleasedMask = 0;
    uint32_t _pressedColor = 0; // GRB packed color

    uint16_t _debounceMs = 0; // 0 = disabled
    uint32_t _lastChangeTime[4] = {0,0,0,0};
};
