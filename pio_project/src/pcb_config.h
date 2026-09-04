#ifndef PCB_CONFIG_H
#define PCB_CONFIG_H

#include <stdint.h>

enum class PcbVersion : uint8_t {
    OLD_PCB = 0,
    NEW_PCB = 1,
};

extern uint8_t gPinLeftA;
extern uint8_t gPinLeftB;
extern uint8_t gPinRightA;
extern uint8_t gPinRightB;
extern uint8_t gSensorSda;
extern uint8_t gSensorScl;

void pcbDetectAndApply();
PcbVersion pcbGetVersion();

#endif
