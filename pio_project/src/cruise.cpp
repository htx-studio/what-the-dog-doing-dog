#include "cruise.h"

#include "config.h"
#include "motor.h"
#include "servo.h"

namespace {
enum class CruiseState : uint8_t {
    IDLE = 0,
    P1_UPPER,
    P1_LOWER,
    P2_UPPER,
    P2_LOWER,
};

constexpr uint8_t PHASE_QUARTER = 64;

CruiseState state = CruiseState::IDLE;
uint32_t segmentStart = 0;
uint16_t segmentDuration = 0;
float targetYaw = 0.0f;
float integral = 0.0f;
float lastError = 0.0f;
int32_t correction = 0;

float normalizeYaw(float value) {
    while (value < 0.0f) value += 360.0f;
    while (value >= 360.0f) value -= 360.0f;
    return value;
}

float angleDifference(float a, float b) {
    float difference = a - b;
    while (difference > 180.0f) difference -= 360.0f;
    while (difference < -180.0f) difference += 360.0f;
    return difference;
}

bool isLeftTwist(CruiseState value) {
    return value == CruiseState::P1_UPPER || value == CruiseState::P2_LOWER;
}

uint16_t clampDuration(int32_t value) {
    if (value < 100) value = 100;
    if (value > 60000) value = 60000;
    return (uint16_t)value;
}

void enterSegment(CruiseState next, uint32_t now) {
    state = next;
    segmentStart = now;
    uint16_t base = isLeftTwist(next)
        ? CRUISE_DEFAULT_TWIST_LEFT_MS
        : CRUISE_DEFAULT_TWIST_RIGHT_MS;
    segmentDuration = clampDuration(
        isLeftTwist(next) ? (int32_t)base + correction
                          : (int32_t)base - correction);
}

uint16_t phaseBase(CruiseState value) {
    switch (value) {
        case CruiseState::P1_UPPER: return 192;
        case CruiseState::P1_LOWER: return 0;
        case CruiseState::P2_UPPER: return 64;
        case CruiseState::P2_LOWER: return 128;
        default: return 0;
    }
}

uint8_t servoPhase(uint32_t now) {
    uint32_t elapsed = now - segmentStart;
    uint32_t progress = segmentDuration
        ? (uint32_t)((uint64_t)elapsed * PHASE_QUARTER / segmentDuration)
        : PHASE_QUARTER;
    if (progress > PHASE_QUARTER) progress = PHASE_QUARTER;
    return (uint8_t)(phaseBase(state) + progress);
}

uint8_t innerSpeed(uint8_t outerSpeed) {
    if (CRUISE_INNER_RATIO_PCT == 0) return 0;
    int32_t speed = (int32_t)outerSpeed * CRUISE_INNER_RATIO_PCT / 100
                  + CRUISE_INNER_PWM_OFFSET;
    if (speed < CRUISE_SPEED_MIN) speed = CRUISE_SPEED_MIN;
    if (speed > PWM_MAX) speed = PWM_MAX;
    return (uint8_t)speed;
}

void updateCorrection(float yawDeg) {
    float error = CRUISE_YAW_POLARITY * angleDifference(targetYaw, yawDeg);
    integral += error;
    if (integral > CRUISE_PID_INT_MAX) integral = CRUISE_PID_INT_MAX;
    if (integral < -CRUISE_PID_INT_MAX) integral = -CRUISE_PID_INT_MAX;
    float derivative = error - lastError;
    lastError = error;
    float output = CRUISE_PID_KP * error
                 + CRUISE_PID_KI * integral
                 + CRUISE_PID_KD * derivative;
    if (output > CRUISE_CORR_MAX) output = CRUISE_CORR_MAX;
    if (output < -CRUISE_CORR_MAX) output = -CRUISE_CORR_MAX;
    correction = (int32_t)output;
}

void advanceSegment(uint32_t now, float yawDeg) {
    switch (state) {
        case CruiseState::P1_UPPER:
            enterSegment(CruiseState::P1_LOWER, now);
            break;
        case CruiseState::P1_LOWER:
            updateCorrection(yawDeg);
            enterSegment(CruiseState::P2_UPPER, now);
            break;
        case CruiseState::P2_UPPER:
            enterSegment(CruiseState::P2_LOWER, now);
            break;
        case CruiseState::P2_LOWER:
            updateCorrection(yawDeg);
            enterSegment(CruiseState::P1_UPPER, now);
            break;
        default:
            break;
    }
}
}

void cruiseInit() {
    state = CruiseState::IDLE;
    targetYaw = 0.0f;
    integral = 0.0f;
    lastError = 0.0f;
    correction = 0;
}

void cruiseEnable(bool enable, float yawDeg) {
    if (!enable) {
        if (state != CruiseState::IDLE) {
            motorCruiseStopBlanking();
            servosToCenter();
        }
        state = CruiseState::IDLE;
        return;
    }

    if (state != CruiseState::IDLE) motorsStop();
    targetYaw = normalizeYaw(yawDeg);
    integral = 0.0f;
    lastError = 0.0f;
    correction = 0;
    enterSegment(CruiseState::P1_UPPER, millis());
}

bool cruiseIsEnabled() {
    return state != CruiseState::IDLE;
}

void cruiseSetTargetYaw(float yawDeg) {
    targetYaw = normalizeYaw(yawDeg);
}

float cruiseGetTargetYaw() {
    return targetYaw;
}

void cruiseUpdate(uint32_t now, float yawDeg) {
    if (state == CruiseState::IDLE) return;
    if (now - segmentStart >= segmentDuration) advanceSegment(now, yawDeg);

    uint8_t outer = CRUISE_DEFAULT_BASE_SPEED;
    uint8_t inner = innerSpeed(outer);
    if (isLeftTwist(state)) {
        setLeftMotor(MOTOR_FORWARD, inner);
        setRightMotor(MOTOR_FORWARD, outer);
    } else {
        setLeftMotor(MOTOR_FORWARD, outer);
        setRightMotor(MOTOR_FORWARD, inner);
    }
    gaitStepCruiseServosAtPhase(servoPhase(now));
}
