#ifndef SERVO_H
#define SERVO_H

#include <Arduino.h>

void servosInit();
void servosToCenter();
void gaitStepCruiseServosAtPhase(uint8_t phase256);

#endif
