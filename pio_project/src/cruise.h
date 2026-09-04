#ifndef CRUISE_H
#define CRUISE_H

#include <Arduino.h>

void cruiseInit();
void cruiseEnable(bool enable, float yawDeg);
bool cruiseIsEnabled();
void cruiseSetTargetYaw(float yawDeg);
float cruiseGetTargetYaw();
void cruiseUpdate(uint32_t now, float yawDeg);

#endif
