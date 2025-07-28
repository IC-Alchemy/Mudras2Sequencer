# MIDI Continuous Controller (CC) Specification

## Overview

This document defines the MIDI CC implementation for real-time parameter control in the PicoMudrasSequencer. The system provides external MIDI control of key synthesis parameters for both Voice 1 and Voice 2.

## CC Number Mappings

### Voice 1 Parameters
| Parameter | CC Number | Description | Range |
|-----------|-----------|-------------|-------|
| Filter    | CC74      | Filter cutoff frequency | 0-127 |
| Attack    | CC73      | Envelope attack time | 0-127 |
| Decay     | CC72      | Envelope decay time | 0-127 |
| Octave    | CC71      | Octave offset | 0-127 |

### Voice 2 Parameters
| Parameter | CC Number | Description | Range |
|-----------|-----------|-------------|-------|
| Filter    | CC78      | Filter cutoff frequency | 0-127 |
| Attack    | CC77      | Envelope attack time | 0-127 |
| Decay     | CC76      | Envelope decay time | 0-127 |
| Octave    | CC75      | Octave offset | 0-127 |

## Channel Assignment Strategy

**Single Channel Approach**: Both Voice 1 and Voice 2 send CC data on **MIDI Channel 1**
- Differentiated by CC number ranges (71-74 for Voice 1, 75-78 for Voice 2)
- Simplifies external DAW/hardware setup
- Maintains compatibility with existing note output

## Value Scaling Formulas

**Important**: All parameters in PicoMudrasSequencer are stored internally as normalized values in the range 0.0f - 1.0f. The scaling to MIDI CC values (0-127) is therefore consistent across all parameter types.

### All Parameters (Filter, Attack, Decay, Octave)
```cpp
// Internal range: 0.0f - 1.0f (normalized)
// MIDI range: 0 - 127
// Simple linear scaling for all parameters
float clampedValue = std::max(0.0f, std::min(parameterValue, 1.0f));
uint8_t midiValue = (uint8_t)(clampedValue * 127.0f);
```

### Parameter-Specific Notes

#### Filter Parameter (CC74/CC78)
- 0.0f = Minimum filter cutoff (MIDI 0)
- 1.0f = Maximum filter cutoff (MIDI 127)

#### Attack Parameter (CC73/CC77)
- 0.0f = Fastest attack time (MIDI 0)
- 1.0f = Slowest attack time (MIDI 127)

#### Decay Parameter (CC72/CC76)
- 0.0f = Fastest decay time (MIDI 0)
- 1.0f = Slowest decay time (MIDI 127)

#### Octave Parameter (CC71/CC75)
- 0.0f = Lowest octave offset (MIDI 0)
- 0.5f = Center/no octave offset (MIDI 64)
- 1.0f = Highest octave offset (MIDI 127)

## Change Detection Algorithm

### Threshold-Based Detection
```cpp
struct CCParameterState {
    float lastValue;
    uint8_t lastMidiValue;
    bool hasChanged;
};

// Change detection with hysteresis
bool hasParameterChanged(float currentValue, CCParameterState& state) {
    // Convert normalized 0.0f-1.0f value to MIDI range 0-127
    float clampedValue = std::max(0.0f, std::min(currentValue, 1.0f));
    uint8_t currentMidiValue = (uint8_t)(clampedValue * 127.0f);

    // Only send if MIDI value actually changed (prevents spam)
    if (currentMidiValue != state.lastMidiValue) {
        state.lastValue = currentValue;
        state.lastMidiValue = currentMidiValue;
        state.hasChanged = true;
        return true;
    }
    return false;
}
```

### Update Frequency Limiting
- Maximum CC transmission rate: 100 Hz (every 10ms)
- Prevents MIDI buffer overflow
- Maintains responsive feel for real-time control

## Integration Points

### 1. Parameter Update Locations
- **onStepCallback()**: Sequencer step advancement
- **updateParametersForStep()**: Real-time parameter recording
- **applyAS5600BaseValues()**: Encoder parameter modifications
- **updateActiveVoiceState()**: Immediate feedback during recording

### 2. Thread Safety Requirements
- CC transmission occurs on Core1 (same as MIDI note output)
- Parameter updates from Core0 use volatile VoiceState structures
- Atomic operations for CC state management

### 3. Data Flow
```
Parameter Change → Change Detection → Value Scaling → CC Transmission
     ↓                    ↓                ↓              ↓
VoiceState Update → Compare with Last → Convert to 0-127 → USB MIDI Out
```

## Implementation Architecture

### MidiCCManager Class Extension
```cpp
class MidiCCManager {
private:
    struct CCParameterState ccStates[2][4]; // [voice][parameter]
    unsigned long lastCCTransmission;

public:
    void updateParameterCC(uint8_t voiceId, ParamId paramId, float value);
    void sendCCIfChanged(uint8_t voiceId, ParamId paramId, float value);
    uint8_t getParameterCCNumber(uint8_t voiceId, ParamId paramId);
    uint8_t scaleParameterToMidi(ParamId paramId, float value);
};
```

### Configuration Constants
```cpp
// CC number base offsets
constexpr uint8_t CC_VOICE1_BASE = 71;
constexpr uint8_t CC_VOICE2_BASE = 75;

// Parameter to CC offset mapping
constexpr uint8_t CC_FILTER_OFFSET = 3;  // CC74/78
constexpr uint8_t CC_ATTACK_OFFSET = 2;  // CC73/77
constexpr uint8_t CC_DECAY_OFFSET = 1;   // CC72/76
constexpr uint8_t CC_OCTAVE_OFFSET = 0;  // CC71/75

// Transmission limits
constexpr unsigned long CC_MIN_INTERVAL_MS = 10;
```

## Testing Strategy

### 1. Unit Testing
- Value scaling accuracy
- Change detection logic
- CC number mapping correctness

### 2. Integration Testing
- Real-time parameter changes
- Multi-voice CC transmission
- Thread safety verification

### 3. External Validation
- DAW CC reception testing
- Hardware synthesizer control
- MIDI monitor verification

## Debug and Monitoring

### Serial Debug Output
```cpp
void debugCCTransmission(uint8_t voice, uint8_t ccNumber, uint8_t value) {
    Serial.print("[CC] Voice ");
    Serial.print(voice + 1);
    Serial.print(" CC");
    Serial.print(ccNumber);
    Serial.print(" = ");
    Serial.println(value);
}
```

### Performance Metrics
- CC transmission frequency
- Change detection efficiency
- MIDI buffer utilization
