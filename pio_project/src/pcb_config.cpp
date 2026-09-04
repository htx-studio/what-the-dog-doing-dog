#include "pcb_config.h"

#include <Arduino.h>

namespace {
constexpr uint8_t DETECT_IO4 = 4;
constexpr uint8_t DETECT_IO5 = 5;
constexpr uint8_t DETECT_SAMPLES = 32;
constexpr uint8_t NEW_MIN_HIGH = 28;
constexpr uint16_t SAMPLE_INTERVAL_US = 100;

PcbVersion detectedVersion = PcbVersion::OLD_PCB;

void applyOldPcbPins() {
    gPinLeftA = 6;
    gPinLeftB = 7;
    gPinRightA = 5;
    gPinRightB = 4;
    gSensorSda = 8;
    gSensorScl = 9;
    detectedVersion = PcbVersion::OLD_PCB;
}

void applyNewPcbPins() {
    gPinLeftA = 7;
    gPinLeftB = 6;
    gPinRightA = 8;
    gPinRightB = 9;
    gSensorSda = 4;
    gSensorScl = 5;
    detectedVersion = PcbVersion::NEW_PCB;
}
}

uint8_t gPinLeftA = 6;
uint8_t gPinLeftB = 7;
uint8_t gPinRightA = 5;
uint8_t gPinRightB = 4;
uint8_t gSensorSda = 8;
uint8_t gSensorScl = 9;

void pcbDetectAndApply() {
    pinMode(DETECT_IO4, INPUT);
    pinMode(DETECT_IO5, INPUT);
    delayMicroseconds(500);

    uint8_t io4High = 0;
    uint8_t io5High = 0;
    for (uint8_t i = 0; i < DETECT_SAMPLES; ++i) {
        if (digitalRead(DETECT_IO4) == HIGH) ++io4High;
        if (digitalRead(DETECT_IO5) == HIGH) ++io5High;
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }

    if (io4High >= NEW_MIN_HIGH && io5High >= NEW_MIN_HIGH) {
        applyNewPcbPins();
    } else {
        applyOldPcbPins();
    }
}

PcbVersion pcbGetVersion() {
    return detectedVersion;
}
