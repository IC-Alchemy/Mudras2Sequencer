#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

#include <Arduino.h>
#include <Melopero_VL53L1X.h>
#include <Wire.h>

/**
 * @class DistanceSensor
 * @brief Minimal VL53L1X distance sensor driver for basic distance measurement
 *
 * Simple, lightweight driver that provides basic distance readings every 20ms
 * without advanced features or optimizations.
 */
class DistanceSensor {
public:
    // Constructor
    DistanceSensor();

    // Initialization with default VL53L1X settings
    bool begin();

    // Main update function - polls sensor every 20ms
    void update();

    // Get raw distance reading in millimeters
    int getRawValue() const;

private:
    // Sensor hardware interface
    Melopero_VL53L1X sensor;

    // Timing control
    unsigned long lastReadTime;

    // Raw sensor data
    int rawMm;

    // Basic timing constant
    static constexpr unsigned long READ_INTERVAL_MS = 20;
};

// Global instance for backward compatibility
extern DistanceSensor distanceSensor;

// Backward compatibility functions
void updateDistanceSensor();

#endif // DISTANCE_SENSOR_H
