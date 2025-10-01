#include <Arduino.h>
#include "HardwareConfig.h"
#include "NeoKeyManager.h"
#include "EncoderManager.h"
#include "StatusLED.h"

NeoKeyManager   g_keypad;
EncoderManager  g_encoder(ENCODER_SWITCH_PIN, ENCODER_PIXEL_PIN);
StatusLED       g_statusLed;

void setup() {
    g_statusLed.begin();
    g_statusLed.setState(StatusLED::State::Waiting);

    Serial.begin(115200);
    while (!Serial) {
        g_statusLed.update();
        delay(5);
    }
    Serial.println();
    Serial.println(F("OpenCANDeck input system starting..."));

    if (!g_keypad.begin(NEOKEY_I2C_ADDR)) {
        Serial.println(F("ERROR: NeoKey init failed."));
        g_statusLed.setState(StatusLED::State::Error);
        while (true) {
            g_statusLed.update();
            delay(100);
        }
    }
    g_keypad.setPressedColor(seesaw_NeoPixel::Color(0, 180, 60));

    if (!g_encoder.begin(ENCODER_I2C_ADDR, ENCODER_PIXEL_BRIGHTNESS)) {
        Serial.println(F("ERROR: Encoder init failed."));
        g_statusLed.setState(StatusLED::State::Error);
        while (true) {
            g_statusLed.update();
            delay(100);
        }
    }

    Serial.println(F("Setup complete."));
    g_statusLed.setState(StatusLED::State::Ok);
}

void loop() {
    g_keypad.update();
    g_encoder.update();
    g_statusLed.update();

    uint8_t jp = g_keypad.justPressed();
    uint8_t jr = g_keypad.justReleased();
    if (jp) {
        for (uint8_t i = 0; i < 4; i++) {
            if (jp & (1 << i)) {
                Serial.print(F("Key ")); Serial.print(i); Serial.println(F(" pressed"));
            }
        }
    }
    if (jr) {
        for (uint8_t i = 0; i < 4; i++) {
            if (jr & (1 << i)) {
                Serial.print(F("Key ")); Serial.print(i); Serial.println(F(" released"));
            }
        }
    }

    delay(INPUT_POLL_DELAY_MS);
}
