#include "servo.h"

#include "config.h"

namespace {
const int16_t SIN_TABLE[256] = {
      0,   6,  12,  18,  25,  31,  37,  43,  49,  55,  62,  68,  74,  80,  86,  92,
     98, 104, 109, 115, 121, 126, 132, 137, 142, 147, 152, 157, 162, 167, 171, 176,
    180, 184, 189, 193, 197, 201, 205, 208, 212, 215, 219, 222, 225, 228, 231, 233,
    236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255,
    256, 255, 255, 255, 254, 254, 253, 252, 251, 249, 248, 246, 244, 243, 241, 238,
    236, 233, 231, 228, 225, 222, 219, 215, 212, 208, 205, 201, 197, 193, 189, 184,
    180, 176, 171, 167, 162, 157, 152, 147, 142, 137, 132, 126, 121, 115, 109, 104,
     98,  92,  86,  80,  74,  68,  62,  55,  49,  43,  37,  31,  25,  18,  12,   6,
      0,  -6, -12, -18, -25, -31, -37, -43, -49, -55, -62, -68, -74, -80, -86, -92,
    -98,-104,-109,-115,-121,-126,-132,-137,-142,-147,-152,-157,-162,-167,-171,-176,
   -180,-184,-189,-193,-197,-201,-205,-208,-212,-215,-219,-222,-225,-228,-231,-233,
   -236,-238,-241,-243,-244,-246,-248,-249,-251,-252,-253,-254,-254,-255,-255,-255,
   -256,-255,-255,-255,-254,-254,-253,-252,-251,-249,-248,-246,-244,-243,-241,-238,
   -236,-233,-231,-228,-225,-222,-219,-215,-212,-208,-205,-201,-197,-193,-189,-184,
   -180,-176,-171,-167,-162,-157,-152,-147,-142,-137,-132,-126,-121,-115,-109,-104,
    -98, -92, -86, -80, -74, -68, -62, -55, -49, -43, -37, -31, -25, -18, -12,  -6
};

const uint8_t SERVO_PINS[SERVO_COUNT] = {
    PIN_SERVO_LEFT_FOOT, PIN_SERVO_RIGHT_FOOT,
    PIN_SERVO_LEFT_HAND, PIN_SERVO_RIGHT_HAND
};
const uint8_t SERVO_CHANNELS[SERVO_COUNT] = {
    PWM_CH_SERVO_LEFT_FOOT, PWM_CH_SERVO_RIGHT_FOOT,
    PWM_CH_SERVO_LEFT_HAND, PWM_CH_SERVO_RIGHT_HAND
};
const uint8_t SERVO_CENTER[SERVO_COUNT] = {
    GAIT_LEFT_FOOT_CENTER, GAIT_RIGHT_FOOT_CENTER,
    GAIT_LEFT_HAND_CENTER, GAIT_RIGHT_HAND_CENTER
};
const uint8_t SERVO_AMPLITUDE[SERVO_COUNT] = {
    GAIT_LEFT_FOOT_AMPLITUDE, GAIT_RIGHT_FOOT_AMPLITUDE,
    GAIT_LEFT_HAND_AMPLITUDE, GAIT_RIGHT_HAND_AMPLITUDE
};
const uint8_t SERVO_PHASE[SERVO_COUNT] = {0, 128, 128, 0};
const bool SERVO_INVERT[SERVO_COUNT] = {true, false, true, false};

uint32_t pulseToDuty(uint32_t microseconds) {
    return microseconds * (1UL << SERVO_BITS) / 20000UL;
}

void setServo(uint8_t index, uint8_t position) {
    if (position > SERVO_POS_DEG_MAX) position = SERVO_POS_DEG_MAX;
    uint32_t pulse = SERVO_PULSE_MIN_US
                   + (uint32_t)(SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)
                   * position / SERVO_POS_DEG_MAX;
    ledcWrite(SERVO_CHANNELS[index], pulseToDuty(pulse));
}
}

void servosInit() {
    for (uint8_t i = 0; i < SERVO_COUNT; ++i) {
        ledcSetup(SERVO_CHANNELS[i], SERVO_FREQ, SERVO_BITS);
        ledcAttachPin(SERVO_PINS[i], SERVO_CHANNELS[i]);
    }
    servosToCenter();
}

void servosToCenter() {
    for (uint8_t i = 0; i < SERVO_COUNT; ++i) {
        setServo(i, SERVO_CENTER[i]);
    }
}

void gaitStepCruiseServosAtPhase(uint8_t phase256) {
    for (uint8_t i = 0; i < SERVO_COUNT; ++i) {
        int32_t wave = SIN_TABLE[(uint8_t)(phase256 + SERVO_PHASE[i])];
        if (SERVO_INVERT[i]) wave = -wave;
        int32_t position = SERVO_CENTER[i] + wave * SERVO_AMPLITUDE[i] / 256;
        if (position < 0) position = 0;
        if (position > SERVO_POS_DEG_MAX) position = SERVO_POS_DEG_MAX;
        setServo(i, (uint8_t)position);
    }
}
