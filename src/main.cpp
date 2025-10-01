#include <Arduino.h>
#include "HardwareConfig.h"
#include "NeoKeyManager.h"
#include "EncoderManager.h"
#include "StatusLED.h"
#include "CANManager.h"

NeoKeyManager g_keypad;
EncoderManager g_encoder(ENCODER_SWITCH_PIN, ENCODER_PIXEL_PIN);
StatusLED g_statusLed;
CANManager g_can;

void setup()
{
    g_statusLed.begin();
    g_statusLed.setState(StatusLED::State::Waiting);

    Serial.begin(115200);
    while (!Serial)
    {
        g_statusLed.update();
        delay(5);
    }
    Serial.println();
    Serial.println(F("OpenCANDeck input system starting..."));

    if (!g_keypad.begin(NEOKEY_I2C_ADDR))
    {
        Serial.println(F("ERROR: NeoKey init failed."));
        g_statusLed.setState(StatusLED::State::Error);
        while (true)
        {
            g_statusLed.update();
            delay(100);
        }
    }
    g_keypad.setPressedColor(seesaw_NeoPixel::Color(0, 180, 60));
    g_keypad.setDebounceTime(50); // 50ms debounce time

    if (!g_encoder.begin(ENCODER_I2C_ADDR, ENCODER_PIXEL_BRIGHTNESS))
    {
        Serial.println(F("ERROR: Encoder init failed."));
        g_statusLed.setState(StatusLED::State::Error);
        while (true)
        {
            g_statusLed.update();
            delay(100);
        }
    }

    // Initialize CAN controller
    if (!g_can.begin(CAN_BAUDRATE))
    {
        Serial.println(F("ERROR: MCP2515 init failed."));
        g_statusLed.setState(StatusLED::State::Error);
        while (true)
        {
            g_statusLed.update();
            delay(100);
        }
    }
    else
    {
        Serial.println(F("MCP2515 CAN controller initialized."));
        // Enable decoded output by default; raw traffic can be toggled later.
        g_can.setDebugDecoded(false);
        g_can.setDebugRaw(false);
    }

    Serial.println(F("Setup complete."));
    g_statusLed.setState(StatusLED::State::Ok);
}

void loop()
{
    static uint32_t lastKeypadMs = 0;
    static uint32_t lastEncoderMs = 0;
    uint32_t now = millis();

    // Fast CAN drain every iteration.
    g_can.poll();

    // Keypad at configured interval
    if (now - lastKeypadMs >= KEYPAD_SCAN_INTERVAL_MS)
    {
        lastKeypadMs = now;
        g_keypad.update();
    }
    // Encoder at configured interval
    if (now - lastEncoderMs >= ENCODER_SCAN_INTERVAL_MS)
    {
        lastEncoderMs = now;
        g_encoder.update();
    }
    // Status LED animation already self-throttles internally
    g_statusLed.update();

    // React to VCFRONT_indicatorLeftRequest: illuminate LED when active.
    if (g_can.hasNewFrontLighting())
    {
        static CANManager::IndicatorLeftReq lastLeft = (CANManager::IndicatorLeftReq)0xFF; // force first update
        auto fl = g_can.getFrontLighting();
        if (fl.indicatorLeftRequest != lastLeft)
        {
            switch (fl.indicatorLeftRequest)
            {
            case CANManager::IndicatorLeftReq::Off:
                g_statusLed.setState(StatusLED::State::Off);
                break;
            case CANManager::IndicatorLeftReq::ActiveLow:
                // Use a dimmer amber to differentiate
                g_statusLed.setColor(120, 40, 0);
                break;
            case CANManager::IndicatorLeftReq::ActiveHigh:
                // Brighter amber
                g_statusLed.setColor(255, 120, 0);
                break;
            case CANManager::IndicatorLeftReq::Unknown:
                // Magenta to indicate unexpected value
                g_statusLed.setColor(200, 0, 150);
                break;
            }
            lastLeft = fl.indicatorLeftRequest;
        }
    }

    uint8_t jp = g_keypad.justPressed();
    uint8_t jr = g_keypad.justReleased();
    if (jp || jr)
    {
        if (jp)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                if (jp & (1 << i))
                {
                    Serial.print(F("Key "));
                    Serial.print(i);
                    Serial.println(F(" pressed"));
                    // Debug toggles
                    if (i == 0)
                    {
                        static bool raw = false;
                        raw = !raw;
                        g_can.setDebugRaw(raw);
                        Serial.print(F("CAN raw debug="));
                        Serial.println(raw ? F("ON") : F("OFF"));
                    }
                    else if (i == 1)
                    {
                        static bool dec = false;
                        dec = !dec;
                        g_can.setDebugDecoded(dec);
                        Serial.print(F("CAN decoded debug="));
                        Serial.println(dec ? F("ON") : F("OFF"));
                    }
                }
            }
        }
        if (jr)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                if (jr & (1 << i))
                {
                    Serial.print(F("Key "));
                    Serial.print(i);
                    Serial.println(F(" released"));
                }
            }
        }
    }
}
