#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

bool sensorsBegin();
void sensorsUpdate(uint32_t now);
float sensorsYaw();

#endif
