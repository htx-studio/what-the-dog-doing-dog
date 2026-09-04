#include "sensors.h"

#include <Wire.h>
#include <Adafruit_AHRS.h>
#include "config.h"
#include "SensorQMC6309.hpp"
#include "SensorQMI8658.hpp"

namespace {
SensorQMC6309 magnetometer;
SensorQMI8658 imu;
Adafruit_Madgwick fusionFilter;
bool magnetometerReady = false;
bool imuReady = false;
uint32_t lastUpdate = 0;
float yaw = 0.0f;
}

bool sensorsBegin() {
    Wire.begin(SENSOR_SDA, SENSOR_SCL, 400000);

    magnetometerReady = magnetometer.begin(
        Wire, QMC6309_SLAVE_ADDRESS, SENSOR_SDA, SENSOR_SCL);
    if (magnetometerReady) {
        magnetometer.configMagnetometer(
            OperationMode::CONTINUOUS_MEASUREMENT,
            MagFullScaleRange::FS_8G,
            200.0f,
            MagOverSampleRatio::OSR_8,
            MagDownSampleRatio::DSR_1);
    }

    imuReady = imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS, SENSOR_SDA, SENSOR_SCL);
    if (imuReady) {
        imu.reset();
        delay(100);
        imu.configAccelerometer(
            SensorQMI8658::ACC_RANGE_8G,
            SensorQMI8658::ACC_ODR_1000Hz,
            SensorQMI8658::LPF_MODE_0);
        imu.configGyroscope(
            SensorQMI8658::GYR_RANGE_512DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_0);
        imu.enableAccelerometer();
        imu.enableGyroscope();
    }

    fusionFilter.begin(1000.0f / SENSOR_UPDATE_INTERVAL_MS);
    fusionFilter.setBeta(FUSION_BETA_NORMAL);
    return imuReady;
}

void sensorsUpdate(uint32_t now) {
    if (!imuReady || now - lastUpdate < SENSOR_UPDATE_INTERVAL_MS) return;
    lastUpdate = now;

    float ax, ay, az, gx, gy, gz;
    if (!imu.getAccelerometer(ax, ay, az)
        || !imu.getGyroscope(gx, gy, gz)) return;

    MagnetometerData magData;
    if (magnetometerReady && magnetometer.readData(magData)) {
        float mx = -magData.magnetic_field.x * 100.0f;
        float my = -magData.magnetic_field.y * 100.0f;
        float mz = magData.magnetic_field.z * 100.0f;
        fusionFilter.update(gx, gy, gz, ax, ay, az, mx, my, mz);
    } else {
        fusionFilter.updateIMU(gx, gy, gz, ax, ay, az);
    }

    yaw = fusionFilter.getYaw();
    while (yaw < 0.0f) yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;
}

float sensorsYaw() {
    return yaw;
}
