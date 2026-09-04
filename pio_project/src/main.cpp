#include <Arduino.h>

#include "config.h"
#include "cruise.h"
#include "motor.h"
#include "pcb_config.h"
#include "sensors.h"
#include "servo.h"
#include "web_control.h"

namespace {
uint32_t lastLedToggle = 0;
bool ledState = false;
}

void setup() {
    pcbDetectAndApply();
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, ledState);
    motorInit();
    motorsStop();
    servosInit();
    cruiseInit();
    sensorsBegin();
    WebControl.begin();
}

void loop() {
    uint32_t now = millis();
    if (now - lastLedToggle >= STATUS_LED_TOGGLE_MS) {
        lastLedToggle = now;
        ledState = !ledState;
        digitalWrite(PIN_STATUS_LED, ledState);
    }
    sensorsUpdate(now);
    WebControl.loop();
    cruiseUpdate(now, sensorsYaw());
    delay(1);
}
