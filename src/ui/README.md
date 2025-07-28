# UI Module

The UI module provides centralized user interface event handling for the PicoMudrasSequencer, managing button interactions, mode switching, and parameter control.

---

## Overview

This module extracts the complex UI event handling logic from the main PicoMudrasSequencer.ino file, providing:

- **Centralized Event Processing**: All matrix button events handled in one place
- **Data-Driven Button Mapping**: Configurable parameter button associations
- **Mode Management**: Voice switching, hold-and-press parameter length setting, and AS5600 control
- **Clean Separation**: UI logic isolated from audio and sequencer cores

---

## Components

### ButtonManager

Manages button state variables, timing arrays, and provides utilities for button press detection.

#### Key Features:
- **Button State Management**: Tracks all parameter button states (16-21)
- **Timing Management**: Handles press timestamps and flash timings
- **Long Press Detection**: Configurable threshold for long press recognition
- **Data-Driven Mappings**: Parameter button associations and mappings

#### Usage:
```cpp
#include "src/ui/ButtonManager.h"

// Initialize button manager
initButtonManager();

// Check if any parameter button is held
if (isAnyParameterButtonHeld()) {
    // Handle parameter recording
}

// Check for long press
if (isLongPress(pressDuration)) {
    // Handle long press action
}
```

### UIEventHandler

Processes all matrix button events with clean separation of concerns.

#### Event Categories:
1. **Parameter Buttons (16-21)**: Note, Velocity, Filter, Attack, Decay, Octave
2. **Mode Buttons (22-25)**: Gate length mode, Parameter length mode, Voice switch, AS5600 control
3. **Step Buttons (0-15)**: Sequencer step interaction and parameter recording
4. **Control Buttons (26-31)**: Play/stop, scale change, theme change, reset, randomize

#### Usage:
```cpp
#include "src/ui/UIEventHandler.h"

// Initialize UI event handler
initUIEventHandler();

// Register with matrix system
Matrix_setEventHandler(matrixEventHandler);
```

---

## Architecture

### Data-Driven Design

The UI module uses data-driven structures for maintainable button mapping:

```cpp
// Parameter button mappings
const ParamButtonMapping PARAM_BUTTON_MAPPINGS[] = {
    {16, ParamId::Note, &button16Held, "Note"},
    {17, ParamId::Velocity, &button17Held, "Velocity"},
    // ... more mappings
};
```

### Event Flow

```
Matrix Button Press
        ↓
matrixEventHandler()
        ↓
┌─────────────────────┐
│ Event Type Check    │
├─────────────────────┤
│ 1. Parameter Button │
│ 2. Mode Button      │
│ 3. Step Button      │
│ 4. Control Button   │
└─────────────────────┘
        ↓
Specific Handler Function
        ↓
Update State & Execute Action
```

### AS5600 Integration

The UI module handles AS5600 magnetic encoder integration:

- **Parameter Auto-Selection**: Automatically selects AS5600 parameter when pressing parameter buttons
- **Double-Press Reset**: Double-press button 25 to reset AS5600 base values
- **Parameter Cycling**: Single-press button 25 to cycle through parameters

---

## Integration

### Main File Integration

To integrate the UI module into the main file:

1. **Include Headers**:
```cpp
#include "src/ui/UIEventHandler.h"
#include "src/ui/ButtonManager.h"
```

2. **Initialize in setup1()**:
```cpp
void setup1() {
    // ... existing setup code ...
    
    // Initialize UI system
    initUIEventHandler();
    
    // Register event handler
    Matrix_setEventHandler(matrixEventHandler);
    
    // ... rest of setup ...
}
```

3. **Remove Extracted Code**: Remove the original matrixEventHandler and related code from main file

### External Dependencies

The UI module requires access to these external components:

- **Sequencer Objects**: `seq1`, `seq2`
- **AS5600 System**: `currentAS5600Parameter`, base values, sensor instance
- **Global State**: `currentScale`, `currentThemeIndex`, `isClockRunning`
- **External Functions**: `updateParametersForStep()`, `onClockStart()`, `onClockStop()`

---

## Benefits

### Code Organization
- **Reduced Main File**: ~300 lines moved from main file
- **Single Responsibility**: Each module handles one aspect of UI
- **Clear Dependencies**: Explicit interfaces between components

### Maintainability
- **Easier Testing**: UI logic can be unit tested independently
- **Clearer Debugging**: UI issues isolated to specific modules
- **Better Documentation**: Focused documentation for UI behavior

### Development Workflow
- **Parallel Development**: UI changes don't affect audio/sequencer code
- **Feature Extensions**: New UI features easily added to dedicated modules
- **Code Reuse**: UI components can be reused in other projects

---

## Constants

### Button Indices
```cpp
constexpr uint8_t BUTTON_GATE_LENGTH_MODE = 22;
constexpr uint8_t BUTTON_VOICE_SWITCH = 24;
constexpr uint8_t BUTTON_AS5600_CONTROL = 25;
constexpr uint8_t BUTTON_PLAY_STOP = 26;
constexpr uint8_t BUTTON_CHANGE_SCALE = 27;
constexpr uint8_t BUTTON_CHANGE_THEME = 28;
constexpr uint8_t BUTTON_RESET_SEQUENCERS = 29;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ1 = 30;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ2 = 31;
```

### Timing
```cpp
constexpr unsigned long LONG_PRESS_THRESHOLD = 400; // ms
constexpr unsigned long AS5600_DOUBLE_PRESS_WINDOW = 500; // ms
constexpr unsigned long CONTROL_LED_FLASH_DURATION_MS = 250;
```

---

## Future Enhancements

- **Gesture Recognition**: Multi-button combinations
- **Customizable Mappings**: Runtime button remapping
- **UI Themes**: Different interaction modes
- **Macro Recording**: Record and playback button sequences

This module provides a solid foundation for UI management while maintaining the real-time performance requirements of the embedded sequencer system.
