#include "DistanceSensor.h"

// Global instance for backward compatibility
DistanceSensor distanceSensor;

DistanceSensor::DistanceSensor()
    : lastReadTime(0)
    , rawMm(0)
{
}

bool DistanceSensor::begin() {
    // Initialize I2C with standard settings
    Wire.begin();
    delay(50); // Give I2C time to stabilize

    // Initialize sensor with default I2C address
    sensor.initI2C(0x29, Wire);

    VL53L1_Error status = sensor.initSensor();
    if (status != VL53L1_ERROR_NONE) {
        Serial.print("VL53L1X init failed with error: ");
        Serial.println(status);
        return false;
    }

    // Configure sensor with default settings
    status = sensor.setDistanceMode(VL53L1_DISTANCEMODE_MEDIUM);
    if (status != VL53L1_ERROR_NONE) return false;

    // Use default 20ms timing budget
    status = sensor.setMeasurementTimingBudgetMicroSeconds(20000);
    if (status != VL53L1_ERROR_NONE) return false;

    // Use default 24ms inter-measurement period
    status = sensor.setInterMeasurementPeriodMilliSeconds(24);
    if (status != VL53L1_ERROR_NONE) return false;

    // Start continuous measurements
    status = sensor.clearInterruptAndStartMeasurement();
    if (status != VL53L1_ERROR_NONE) return false;

    Serial.println("VL53L1X distance sensor initialized");
    return true;
}

void DistanceSensor::update() {
    unsigned long currentTime = millis();

    // Simple polling with 20ms interval
    if (currentTime - lastReadTime < READ_INTERVAL_MS) {
        return;
    }
    lastReadTime = currentTime;

    // Use a timeout approach to avoid blocking LEDs
    // Try to get measurement data with a short timeout
    unsigned long startTime = millis();
    VL53L1_Error status = VL53L1_ERROR_NONE;

    // Wait for data with maximum 5ms timeout to avoid blocking LEDs
    while ((millis() - startTime) < 5) {
        status = sensor.waitMeasurementDataReady();
        if (status == VL53L1_ERROR_NONE) {
            break; // Data is ready
        }
    }

    // If we timed out, skip this reading
    if (status != VL53L1_ERROR_NONE) {
        return;
    }

    // Get measurement data
    status = sensor.getRangingMeasurementData();
    if (status != VL53L1_ERROR_NONE) {
        return;
    }

    // Start next measurement
    sensor.clearInterruptAndStartMeasurement();

    // Store raw reading
    rawMm = sensor.measurementData.RangeMilliMeter;
}

// Get raw distance reading in millimeters
int DistanceSensor::getRawValue() const {
    return rawMm;
}

// Backward compatibility function
void updateDistanceSensor() {
    distanceSensor.update();
}
