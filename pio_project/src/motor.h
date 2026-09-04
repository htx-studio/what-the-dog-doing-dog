#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

enum MotorDir : uint8_t {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_REVERSE,
};

void motorInit();
void setLeftMotor(MotorDir dir, uint8_t duty);
void setRightMotor(MotorDir dir, uint8_t duty);
void motorsStop();
void motorCruiseStopBlanking();

#endif
