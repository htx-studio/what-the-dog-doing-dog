#include "motor.h"

#include "config.h"

namespace {
struct MotorState {
    MotorDir direction = MOTOR_STOP;
};

MotorState leftMotor;
MotorState rightMotor;

void bindPwmPin(uint8_t pwmPin, uint8_t lowPin, uint8_t channel) {
    ledcDetachPin(lowPin);
    pinMode(lowPin, OUTPUT);
    digitalWrite(lowPin, LOW);
    ledcAttachPin(pwmPin, channel);
}

void stopMotorPins(uint8_t pinA, uint8_t pinB, uint8_t channel) {
    ledcWrite(channel, 0);
    ledcDetachPin(pinA);
    ledcDetachPin(pinB);
    pinMode(pinA, OUTPUT);
    pinMode(pinB, OUTPUT);
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
}

void setMotor(MotorState& state, uint8_t pinA, uint8_t pinB,
              uint8_t channel, MotorDir direction, uint8_t duty) {
    uint8_t output = direction == MOTOR_STOP ? 0 : duty;
    if (direction == state.direction && direction != MOTOR_STOP) {
        ledcWrite(channel, output);
        return;
    }

    if (direction == MOTOR_FORWARD) {
        bindPwmPin(pinA, pinB, channel);
        ledcWrite(channel, output);
    } else if (direction == MOTOR_REVERSE) {
        bindPwmPin(pinB, pinA, channel);
        ledcWrite(channel, output);
    } else {
        stopMotorPins(pinA, pinB, channel);
    }
    state.direction = direction;
}
}

void motorInit() {
    pinMode(PIN_LEFT_A, OUTPUT);
    pinMode(PIN_LEFT_B, OUTPUT);
    pinMode(PIN_RIGHT_A, OUTPUT);
    pinMode(PIN_RIGHT_B, OUTPUT);
    digitalWrite(PIN_LEFT_A, LOW);
    digitalWrite(PIN_LEFT_B, LOW);
    digitalWrite(PIN_RIGHT_A, LOW);
    digitalWrite(PIN_RIGHT_B, LOW);
    ledcSetup(PWM_CH_LEFT, PWM_FREQ, PWM_BITS);
    ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_BITS);
}

void setLeftMotor(MotorDir direction, uint8_t duty) {
    setMotor(leftMotor, PIN_LEFT_A, PIN_LEFT_B, PWM_CH_LEFT, direction, duty);
}

void setRightMotor(MotorDir direction, uint8_t duty) {
    setMotor(rightMotor, PIN_RIGHT_A, PIN_RIGHT_B, PWM_CH_RIGHT, direction, duty);
}

void motorsStop() {
    setLeftMotor(MOTOR_STOP, 0);
    setRightMotor(MOTOR_STOP, 0);
}

void motorCruiseStopBlanking() {
    ledcWrite(PWM_CH_LEFT, 0);
    ledcWrite(PWM_CH_RIGHT, 0);
    ledcDetachPin(PIN_LEFT_A);
    ledcDetachPin(PIN_LEFT_B);
    ledcDetachPin(PIN_RIGHT_A);
    ledcDetachPin(PIN_RIGHT_B);
    pinMode(PIN_LEFT_A, OUTPUT);
    pinMode(PIN_LEFT_B, OUTPUT);
    pinMode(PIN_RIGHT_A, OUTPUT);
    pinMode(PIN_RIGHT_B, OUTPUT);
    digitalWrite(PIN_LEFT_A, HIGH);
    digitalWrite(PIN_LEFT_B, HIGH);
    digitalWrite(PIN_RIGHT_A, HIGH);
    digitalWrite(PIN_RIGHT_B, HIGH);
#if MOTOR_CRUISE_STOP_BLANK_US > 0
    delayMicroseconds(MOTOR_CRUISE_STOP_BLANK_US);
#endif
    digitalWrite(PIN_LEFT_A, LOW);
    digitalWrite(PIN_LEFT_B, LOW);
    digitalWrite(PIN_RIGHT_A, LOW);
    digitalWrite(PIN_RIGHT_B, LOW);
    leftMotor.direction = MOTOR_STOP;
    rightMotor.direction = MOTOR_STOP;
}
