# LED Controller Module

The LED Controller module provides centralized control LED management for the PicoMudrasSequencer, handling parameter button feedback, mode indicators, voice selection, and AS5600 encoder visual feedback.

---

## Overview

This module extracts the LED control logic from the main PicoMudrasSequencer.ino file, providing:

- **Centralized LED Management**: All control LED logic in one focused module
- **AS5600 Integration**: Dynamic visual feedback for magnetic encoder parameter control
- **Data-Driven Configuration**: Configurable LED mappings and color schemes
- **Real-Time Performance**: Optimized for millisecond-level LED updates

---

## Components

### LEDController.h/.cpp

Manages all control LED functionality including:

#### Key Features:
- **Parameter Button LEDs**: Visual feedback for buttons 16-21 (Note, Velocity, Filter, Attack, Decay, Octave)
- **Mode Indicator LEDs**: Gate mode, Parameter mode, Voice selection indicators
- **AS5600 Visual Feedback**: Dynamic flash speeds based on encoder boundary proximity
- **Flash Timing Management**: Coordinated LED animations and timing

#### LED Index Constants:
```cpp
namespace ControlLEDs {
    // Parameter button LEDs (buttons 16-21)
    constexpr int NOTE_LED = 48;
    constexpr int VELOCITY_LED = 49;
    constexpr int FILTER_LED = 50;
    constexpr int ATTACK_LED = 51;
    constexpr int DECAY_LED = 52;
    constexpr int OCTAVE_LED = 53;
    
    // Mode indicator LEDs
    constexpr int GATE_MODE_LED = 54;
    constexpr int PARAM_MODE_LED = 55;
    constexpr int VOICE1_LED = 56;
    constexpr int VOICE2_LED = 57;
    constexpr int RANDOMIZE_LED = 63;
}
```

---

## Architecture

### LED Update Flow

```
loop1() (Core1)
        ↓
updateControlLEDs(ledMatrix)
        ↓
┌─────────────────────────┐
│ Calculate Timing Values │
├─────────────────────────┤
│ • Standard pulse        │
│ • Dynamic AS5600 pulse  │
│ • Flash timing checks   │
└─────────────────────────┘
        ↓
┌─────────────────────────┐
│ Update LED Categories   │
├─────────────────────────┤
│ • Parameter buttons     │
│ • Mode indicators       │
│ • Voice selection       │
│ • Randomize button      │
└─────────────────────────┘
        ↓
LED Matrix Output
```

### AS5600 Integration

The LED controller provides dynamic visual feedback for AS5600 magnetic encoder control:

- **Parameter Auto-Selection**: LEDs highlight the currently selected AS5600 parameter
- **Boundary Proximity Feedback**: Flash speed increases as encoder values approach ±0.8 limits
- **Dynamic Pulse Rates**: Normal (1x), Warning (2x), Critical (3x) flash speeds

---

## Integration

### Main File Integration

The LED controller is integrated into the main file as follows:

1. **Include Header**:
```cpp
#include "src/LEDMatrix/LEDController.h"
```

2. **Initialize in setup1()**:
```cpp
void setup1() {
    // ... existing setup code ...
    
    ledMatrix.begin(200);
    setupLEDMatrixFeedback();
    initLEDController();  // Initialize LED controller
    
    // ... rest of setup ...
}
```

3. **Update in loop1()**:
```cpp
void loop1() {
    // ... existing loop code ...
    
    updateControlLEDs(ledMatrix);  // Update all control LEDs
    ledMatrix.show();
    
    // ... rest of loop ...
}
```

### External Dependencies

The LED controller requires access to these external components:

- **Button States**: `button16Held` through `button21Held` from ButtonManager
- **Mode States**: `modGateParamSeqLengthsMode`, `isVoice2Mode`
- **AS5600 System**: `currentAS5600Parameter`, `as5600Sensor` instance
- **Flash Timing**: `flash31Until` for randomize button animation
- **External Functions**: `calculateDynamicFlashSpeed()`, `getActiveThemeColors()`

---

## Benefits

### Code Organization
- **Reduced Main File**: ~135 lines moved from main file to focused module
- **Single Responsibility**: LED controller handles only LED-related functionality
- **Clear Dependencies**: Explicit interfaces with other modules

### Performance
- **Real-Time Updates**: Maintains millisecond-level LED update frequency
- **Cached Calculations**: Time-based values calculated once per update cycle
- **Optimized Lambdas**: Efficient color calculation and LED indexing

### Maintainability
- **Data-Driven Design**: LED configurations stored in structured arrays
- **Centralized Constants**: All LED indices and timing values in one place
- **Modular Architecture**: Easy to modify LED behavior without affecting other systems

---

## LED Functionality

### Parameter Button LEDs (16-21)
- **Idle State**: Dim color indicating parameter type
- **Held State**: Bright color when button is pressed
- **AS5600 Selected**: Dynamic pulsing with boundary proximity feedback
- **Length Edit Mode**: Special pulsing when in parameter length editing mode

### Mode Indicator LEDs
- **Parameter Mode (LED 55)**: Pulses when in parameter length editing mode
- **Gate Mode (LED 54)**: Pulses when in gate length editing mode
- **Voice Selection (LEDs 56-57)**: Shows active voice (Voice 1 or Voice 2)

### Randomize LED (63)
- **Flash Animation**: Bright flash when randomize buttons are pressed
- **Timing Control**: Flash duration controlled by `CONTROL_LED_FLASH_DURATION_MS`

---

## Performance Considerations

### Real-Time Requirements
- **Update Frequency**: Called every millisecond in `loop1()` on Core1
- **Calculation Efficiency**: Time-based values cached for performance
- **Memory Usage**: Minimal memory footprint with stack-allocated structures

### Thread Safety
- **Core1 Execution**: All LED updates run on Core1 (UI/sequencer core)
- **No Core0 Impact**: Zero impact on Core0 audio processing
- **Atomic Operations**: LED updates are atomic within single function call

---

## Future Enhancements

### Potential Improvements
- **LED Brightness Control**: Global brightness adjustment
- **Custom Animation Patterns**: User-configurable LED animations
- **Color Customization**: Runtime color theme switching
- **Performance Monitoring**: LED update timing diagnostics

### Integration Opportunities
- **MIDI Feedback**: LED responses to incoming MIDI messages
- **Sensor Integration**: Additional sensor-driven LED feedback
- **Network Control**: Remote LED control via network protocols
