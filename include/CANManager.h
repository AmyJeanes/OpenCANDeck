// Simple wrapper around Adafruit_MCP2515 for reception (and future transmit)
#pragma once
#include <Adafruit_MCP2515.h>
#include <Arduino.h>
#include "HardwareConfig.h"

class CANManager
{
public:
    // Enum for VCFRONT_indicatorLeftRequest
    enum class IndicatorLeftReq : uint8_t
    {
        Off = 0,        // TURN_SIGNAL_OFF
        ActiveLow = 1,  // TURN_SIGNAL_ACTIVE_LOW
        ActiveHigh = 2, // TURN_SIGNAL_ACTIVE_HIGH
        Unknown = 3     // SNA / invalid (value 3 not defined in VAL table)
    };

    // Decoded subset of VCRIGHT_doorStatus (CAN ID 0x103) needed for LED binding.
    struct RightDoorStatusMsg
    {
        bool rearIntSwitchPressed = false; // VCRIGHT_rearIntSwitchPressed (32|1@1+)
        uint32_t lastRxMs = 0;             // millis() timestamp when received
    };

    // Decoded subset of VCFRONT_lighting (CAN ID 0x3F5) focusing on indicator left request.
    // Signal definition: VCFRONT_indicatorLeftRequest : 0|2@1+ (1,0) [0|2]
    // 0 = TURN_SIGNAL_OFF, 1 = TURN_SIGNAL_ACTIVE_LOW, 2 = TURN_SIGNAL_ACTIVE_HIGH (treat 1/2 as active)
    struct FrontLightingMsg
    {
        IndicatorLeftReq indicatorLeftRequest = IndicatorLeftReq::Off;
        uint32_t lastRxMs = 0;
    };

    explicit CANManager(uint8_t csPin = PIN_CAN_CS) : _mcp(csPin) {}

    bool begin(uint32_t bitrate = CAN_BAUDRATE)
    {
        if (!_mcp.begin(bitrate))
            return false;

        // Filter for exactly 0x103 (door status) and 0x3F5 (front lighting)
        // All other messages are ignored on the first mask
        if (!_mcp.setFilterMask(0, false, 0x7FF) ||
            !_mcp.setFilter(0, false, 0x103) ||
            !_mcp.setFilter(1, false, 0x3F5))
        {
            return false;
        }

        // Filter out all messages on the second mask
        if (!_mcp.setFilterMask(1, false, 0x7FF) ||
            !_mcp.setFilter(2, false, 0x000) ||
            !_mcp.setFilter(3, false, 0x000) ||
            !_mcp.setFilter(4, false, 0x000) ||
            !_mcp.setFilter(5, false, 0x000))
        {
            return false;
        }

        return true;
    }

    void setDebugRaw(bool enabled) { _debugRaw = enabled; }
    void setDebugDecoded(bool enabled) { _debugDecoded = enabled; }

    bool hasNewRightDoorStatus() const { return _rightDoorNew; }
    RightDoorStatusMsg getRightDoorStatus(bool clear = true)
    {
        RightDoorStatusMsg c = _rightDoor;
        if (clear)
            _rightDoorNew = false;
        return c;
    }

    bool hasNewFrontLighting() const { return _frontLightingNew; }
    FrontLightingMsg getFrontLighting(bool clear = true)
    {
        FrontLightingMsg c = _frontLighting;
        if (clear)
            _frontLightingNew = false;
        return c;
    }

    // Poll and drain all pending frames; returns true if at least one processed.
    bool poll()
    {
        bool any = false;
        // Drain loop: parsePacket() returns 0 when no more frames.
        while (true)
        {
            int packetSize = _mcp.parsePacket();
            if (!packetSize)
                break;
            any = true;

            uint32_t id = _mcp.packetId();
            bool isRtr = _mcp.packetRtr();
            uint8_t data[8] = {0};
            int len = 0;
            if (!isRtr)
            {
                while (_mcp.available() && len < 8)
                {
                    data[len++] = _mcp.read();
                }
            }

            if (_debugRaw)
            {
                Serial.print(F("CAN: id=0x"));
                Serial.print(id, HEX);
                if (_mcp.packetExtended())
                    Serial.print(F(" ext"));
                if (isRtr)
                    Serial.print(F(" RTR"));
                Serial.print(F(" len="));
                Serial.print(packetSize);
                Serial.print(F(" data="));
                if (isRtr)
                {
                    Serial.print(F("<RTR>"));
                }
                else
                {
                    for (int i = 0; i < len; ++i)
                    {
                        if (data[i] < 0x10)
                            Serial.print('0');
                        Serial.print(data[i], HEX);
                        Serial.print(' ');
                    }
                }
                Serial.println();
            }

            // Decode VCRIGHT_doorStatus (standard ID 0x103, DLC 8)
            if (id == 0x103 && !isRtr && len >= 5)
            { // need at least byte 4 for rearIntSwitchPressed
                RightDoorStatusMsg msg;
                // Signal VCRIGHT_rearIntSwitchPressed : 32|1@1+ (little-endian) -> bit0 (LSB) of byte 4
                msg.rearIntSwitchPressed = (data[4] & 0x01) != 0;
                msg.lastRxMs = millis();
                _rightDoor = msg;
                _rightDoorNew = true;
                if (_debugDecoded)
                {
                    Serial.print(F("DoorStatus: rearIntSwitchPressed="));
                    Serial.print(msg.rearIntSwitchPressed);
                    Serial.println();
                }
            }

            // Decode VCFRONT_lighting (standard ID 0x3F5, DLC 8) - only need first byte for indicatorLeftRequest
            if (id == 0x3F5 && !isRtr && len >= 1)
            {
                FrontLightingMsg msg;
                uint8_t raw = (uint8_t)(data[0] & 0x03); // bits 0-1
                switch (raw)
                {
                case 0:
                    msg.indicatorLeftRequest = IndicatorLeftReq::Off;
                    break;
                case 1:
                    msg.indicatorLeftRequest = IndicatorLeftReq::ActiveLow;
                    break;
                case 2:
                    msg.indicatorLeftRequest = IndicatorLeftReq::ActiveHigh;
                    break;
                default:
                    msg.indicatorLeftRequest = IndicatorLeftReq::Unknown;
                    break;
                }
                msg.lastRxMs = millis();
                _frontLighting = msg;
                _frontLightingNew = true;
                if (_debugDecoded)
                {
                    Serial.print(F("FrontLighting: indicatorLeftRequest="));
                    switch (msg.indicatorLeftRequest)
                    {
                    case IndicatorLeftReq::Off:
                        Serial.println(F("OFF"));
                        break;
                    case IndicatorLeftReq::ActiveLow:
                        Serial.println(F("ACTIVE_LOW"));
                        break;
                    case IndicatorLeftReq::ActiveHigh:
                        Serial.println(F("ACTIVE_HIGH"));
                        break;
                    case IndicatorLeftReq::Unknown:
                        Serial.println(F("UNKNOWN"));
                        break;
                    }
                }
            }
        }
        return any;
    }

    Adafruit_MCP2515 &mcp() { return _mcp; }

private:
    Adafruit_MCP2515 _mcp;
    bool _debugRaw = false;
    bool _debugDecoded = true; // default show decoded message when present
    RightDoorStatusMsg _rightDoor{};
    bool _rightDoorNew = false;
    FrontLightingMsg _frontLighting{};
    bool _frontLightingNew = false;
};
