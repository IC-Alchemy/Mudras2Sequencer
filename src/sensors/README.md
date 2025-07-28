# Sensors Module

This module provides sensor drivers for gesture-based parameter control in the PicoMudrasSequencer. It includes drivers for the VL53L1X Time-of-Flight distance sensor and AS5600 magnetic rotary encoder, both optimized for real-time music performance.

---

## Overview

The sensors module implements non-blocking sensor drivers that integrate seamlessly with the dual-core architecture. Sensors provide continuous parameter control through hand gestures (distance sensor) and rotary manipulation (magnetic encoder), with velocity-sensitive scaling and visual feedback.

### Design Philosophy

- **Non-Blocking Operation**: All sensor reads use timeouts to prevent blocking LED updates
- **Real-Time Performance**: 20ms update intervals for responsive parameter control
- **Gesture-Based Interface**: Natural hand movements translate to musical parameter changes
- **Velocity Sensitivity**: Encoder speed affects parameter modification scaling (1280× dynamic range)

---

## Components

### 1. DistanceSensor Class (VL53L1X)

Provides millimeter-precision distance measurement for gesture-based parameter control.

#### Key Features

- **Range**: 40-400mm optimal sensing distance
- **Precision**: ±3mm accuracy in medium-distance mode
- **Update Rate**: 20ms intervals with 5ms timeout protection
- **Integration**: Direct mapping to parameter ranges with real-time feedback

#### Implementation

<augment_code_snippet path="src/sensors/DistanceSensor.h" mode="EXCERPT">
````cpp
class DistanceSensor {
public:
    bool begin();                    // Initialize sensor
    void update();                   // Non-blocking update (20ms intervals)
    int getRawValue() const;         // Get distance in millimeters

private:
    Melopero_VL53L1X sensor;
    int rawMm;
    unsigned long lastReadTime;
    static constexpr unsigned long READ_INTERVAL_MS = 20;
};
````
</augment_code_snippet>

#### Configuration

<augment_code_snippet path="src/sensors/DistanceSensor.cpp" mode="EXCERPT">
````cpp
bool DistanceSensor::begin() {
    if (!sensor.begin()) return false;

    // Configure for balanced accuracy/speed
    sensor.setDistanceMode(VL53L1_DISTANCEMODE_MEDIUM);
    sensor.setMeasurementTimingBudgetMicroSeconds(20000);  // 20ms timing
    sensor.setInterMeasurementPeriodMilliSeconds(24);     // 24ms period

    return true;
}
````
</augment_code_snippet>

### 2. AS5600Sensor Class (Magnetic Encoder)

Provides continuous rotary control with velocity-sensitive parameter modification.

### 3. AS5600Manager

This module, comprised of `AS5600Manager.h` and `AS5600Manager.cpp`, centralizes the logic for handling the AS5600 magnetic encoder's data. It's responsible for updating and applying the encoder's values to the synthesizer's parameters.

#### Key Features

- **Bidirectional Velocity-Sensitive Control**: The speed and direction of the encoder's rotation are used to modify parameter values.
- **Parameter Application**: Applies encoder values to voice, delay, and LFO parameters.
- **Thread-Safe**: Designed to work in a dual-core environment, with thread-safe communication between cores.
- **Range Clamping**: Ensures that parameter values stay within their valid ranges.


#### Key Features

- **Resolution**: 12-bit (4096 positions per revolution)
- **Velocity Scaling**: 1280× dynamic range (0.001 to 1.28 scaling factors)
- **Parameter Range**: ±0.8 modification range preserving sequencer step contribution
- **Smooth Curves**: Polynomial scaling for natural encoder feel
- **Visual Feedback**: LED flash speed indicates proximity to parameter boundaries
- **Reliable**: Uses standard VL53L1X settings for consistent operation
- **Easy Integration**: Simple API with minimal configuration required

#### Performance Specifications

| Metric | Value | Notes |
|--------|-------|-------|
| **Update Rate** | ~42Hz | 20ms polling interval |
| **Timing Budget** | 20ms | Standard VL53L1X timing |
| **Inter-measurement Period** | 24ms | Standard VL53L1X period |
| **I2C Frequency** | 100kHz | Standard I2C speed |
| **Processing** | None | Raw readings only |

---

## API Reference

### Initialization

```cpp
#include "src/sensors/DistanceSensor.h"

// Initialize with default VL53L1X settings
bool success = distanceSensor.begin();
```

### Basic Usage

```cpp
void loop() {
    // Update sensor (polls every 20ms)
    distanceSensor.update();

    // Get raw distance reading in millimeters
    int distance = distanceSensor.getRawValue();
}
```

### Complete Example

```cpp
#include "src/sensors/DistanceSensor.h"

void setup() {
    Serial.begin(115200);

    if (!distanceSensor.begin()) {
        Serial.println("Sensor initialization failed!");
        while(1);
    }

    Serial.println("Distance sensor ready");
}

void loop() {
    distanceSensor.update();

    int distance = distanceSensor.getRawValue();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    delay(100); // Optional additional delay
}
```

---

## Integration with Sequencer

The distance sensor integrates with the sequencer's parameter recording system:

### Data Flow

```
Hand Gesture → VL53L1X → Raw Reading → Normalize → Parameter Recording
```

### Sequencer Integration

```cpp
// In sequencer parameter recording
if (parameterButtonHeld) {
    int rawDistance = distanceSensor.getRawValue();
    // Simple normalization (done in sequencer code)
    float normalizedValue = (float)rawDistance / MAX_RANGE;
    float paramValue = mapNormalizedValueToParamRange(paramId, normalizedValue);
    sequencer.setStepParameterValue(paramId, stepIndex, paramValue);
}
```

### Backward Compatibility

The module provides backward compatibility for existing code:

```cpp
// Legacy function still works
updateDistanceSensor();  // Calls distanceSensor.update()
```

---

## Design Philosophy

### Simplicity First

The sensor driver prioritizes simplicity and reliability:

- **Minimal Code**: Only essential functionality included
- **Standard Settings**: Uses default VL53L1X configuration
- **No Processing**: Raw readings without filtering or smoothing
- **Easy Debugging**: Simple code path for troubleshooting

### Memory Efficiency

- **Static Allocation**: All variables use stack/static memory
- **Minimal Footprint**: Small memory usage
- **No Dependencies**: Only requires VL53L1X library

---

## Hardware Requirements

### VL53L1X Connections

| VL53L1X Pin | Pico Pin | Description |
|-------------|----------|-------------|
| VCC | 3.3V | Power supply |
| GND | GND | Ground |
| SDA | GPIO 4 (SDA) | I2C Data |
| SCL | GPIO 5 (SCL) | I2C Clock |
| GPIO1 | GPIO 2 (optional) | Interrupt output |

### Optional Interrupt Mode

For maximum efficiency, connect the VL53L1X GPIO1 pin to a Pico GPIO pin and enable interrupt mode:

```cpp
distanceSensor.enableInterruptMode(2); // Use GPIO 2 for interrupts
```

---

## Troubleshooting

### Common Issues

1. **Sensor Not Detected**
   - Check I2C connections (SDA/SCL)
   - Verify power supply (3.3V)
   - Ensure correct I2C address (default: 0x29)

2. **Inconsistent Readings**
   - Check for reflective surfaces in sensor range
   - Ensure stable mounting of sensor
   - Verify adequate lighting conditions

3. **No Readings**
   - Check if `begin()` returned true
   - Verify sensor is within measurement range (up to ~4m)
   - Ensure target object is not too small or transparent

### Debug Output

Add simple debug output to monitor sensor:

```cpp
void loop() {
    distanceSensor.update();

    int distance = distanceSensor.getRawValue();
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    delay(500); // Print every 500ms
}
```

---

## Dependencies

- **Melopero_VL53L1X**: VL53L1X sensor library
- **Wire**: Arduino I2C library
- **Arduino Core**: Standard Arduino functions

---

## Specifications

### Technical Details
- **Sensor**: VL53L1X Time-of-Flight
- **Range**: Up to 4 meters
- **Accuracy**: ±3% typical
- **Update Rate**: ~42Hz (20ms polling)
- **I2C Address**: 0x29 (default)
- **Power**: 3.3V

### Code Metrics
- **Header Size**: ~50 lines
- **Implementation**: ~85 lines
- **Memory Usage**: <100 bytes
- **CPU Usage**: Minimal

**Design Goal**: Simple, reliable distance measurement without complexity.
