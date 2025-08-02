# Settings Mode Implementation Plan

## Overview
Implement a user settings mode accessible when the sequencer is stopped, allowing users to view and change voice presets for voice1 and voice2 using the first 8 step buttons for navigation.

## Current System Analysis

### Existing Components
- **OLED Display**: 128x64 SH1106G display with existing update system
- **UI State Management**: Centralized UIState struct in `UIState.h`
- **Button System**: Matrix-based button handling via `UIEventHandler.cpp`
- **Voice System**: Voice presets available in `Voice.cpp` namespace `VoicePresets`
- **Clock System**: `isClockRunning` boolean tracks sequencer state

### Available Voice Presets
1. Analog Voice (`getAnalogVoice()`)
2. Digital Voice (`getDigitalVoice()`)
3. Bass Voice (`getBassVoice()`)
4. Lead Voice (`getLeadVoice()`)
5. Pad Voice (`getPadVoice()`)
6. Percussion Voice (`getPercussionVoice()`)

### Current PLAY_STOP Button Behavior
- Located at `BUTTON_PLAY_STOP` (index 26)
- Toggles `isClockRunning` state
- Calls `onClockStart()` or `onClockStop()`
- Flashes LED for 250ms

## Implementation Plan

### Phase 1: UI State Extensions

#### 1.1 Extend UIState Structure
Add new fields to `UIState.h`:
```cpp
// Settings mode state
bool settingsMode = false;
uint8_t settingsMenuIndex = 0;  // 0-7 for 8 menu items
uint8_t settingsSubMenuIndex = 0; // For preset selection
bool inPresetSelection = false;
uint8_t voice1PresetIndex = 3;  // Default to Lead Voice
uint8_t voice2PresetIndex = 2;  // Default to Bass Voice
```

#### 1.2 Settings Menu Structure
Define menu items (0-7 corresponding to step buttons 1-8):
- **0**: Voice 1 Preset
- **1**: Voice 2 Preset
- **2**: Global Volume
- **3**: MIDI Channel
- **4**: Scale Lock
- **5**: Auto-Save
- **6**: Factory Reset
- **7**: Exit Settings

### Phase 2: Button Event Handling Modifications

#### 2.1 Modify PLAY_STOP Button Logic
Update `handleControlButtonEvent()` in `UIEventHandler.cpp`:
```cpp
case BUTTON_PLAY_STOP:
    if (isClockRunning) {
        onClockStop();
    } else {
        // Check if we should enter settings mode
        if (!uiState.settingsMode) {
            // Long press detection needed here
            onClockStart();
        }
    }
    uiState.flash25Until = millis() + CONTROL_LED_FLASH_DURATION_MS;
    break;
```

#### 2.2 Add Settings Mode Entry Logic
Implement long-press detection for PLAY_STOP when sequencer is stopped:
- Short press: Start sequencer (existing behavior)
- Long press (>500ms): Enter settings mode

#### 2.3 Step Button Handling in Settings Mode
Modify `handleStepButtonEvent()` to handle settings navigation:
```cpp
if (uiState.settingsMode) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
        if (evt.buttonIndex < 8) { // Only first 8 buttons
            if (uiState.inPresetSelection) {
                handlePresetSelection(evt.buttonIndex, uiState);
            } else {
                handleSettingsMenuNavigation(evt.buttonIndex, uiState);
            }
        }
    }
    return true;
}
```

### Phase 3: OLED Display Updates

#### 3.1 Settings Menu Display
Add new display mode to `OLEDDisplay::update()`:
```cpp
if (uiState.settingsMode) {
    displaySettingsMenu(uiState);
    return;
}
```

#### 3.2 Settings Menu Layout
```
┌──────────────────────────┐
│        Settings          │
├──────────────────────────┤
│ 1. Voice 1: Lead      >  │
│ 2. Voice 2: Bass      >  │
│ 3. Global Vol: 80%       │
│ 4. MIDI Ch: 1            │
│ 5. Scale Lock: Off       │
│ 6. Auto-Save: On         │
│ 7. Factory Reset         │
│ 8. Exit Settings         │
└──────────────────────────┘
```

#### 3.3 Preset Selection Display
```
┌──────────────────────────┐
│     Voice 1 Preset       │
├──────────────────────────┤
│ 1. Analog                │
│ 2. Digital               │
│ 3. Bass                  │
│ 4. Lead              ●   │
│ 5. Pad                   │
│ 6. Percussion            │
│ 7. Back                  │
│ 8. Apply                 │
└──────────────────────────┘
```

### Phase 4: Voice Preset Management

#### 4.1 Preset Name Array
Create preset name mapping:
```cpp
static const char* VOICE_PRESET_NAMES[] = {
    "Analog",
    "Digital", 
    "Bass",
    "Lead",
    "Pad",
    "Percussion"
};
```

#### 4.2 Preset Application Functions
Add functions to apply presets:
```cpp
void applyVoicePreset(uint8_t voiceId, uint8_t presetIndex) {
    VoiceConfig config;
    switch(presetIndex) {
        case 0: config = VoicePresets::getAnalogVoice(); break;
        case 1: config = VoicePresets::getDigitalVoice(); break;
        case 2: config = VoicePresets::getBassVoice(); break;
        case 3: config = VoicePresets::getLeadVoice(); break;
        case 4: config = VoicePresets::getPadVoice(); break;
        case 5: config = VoicePresets::getPercussionVoice(); break;
    }
    voiceManager->setVoiceConfig(voiceId, config);
}
```

### Phase 5: LED Feedback

#### 5.1 Settings Mode LED Indicators
- Step buttons 1-8: Illuminate to show available options
- Current selection: Brighter/different color
- PLAY_STOP button: Slow pulse to indicate settings mode

#### 5.2 LED Update Logic
Modify LED update functions to handle settings mode:
```cpp
if (uiState.settingsMode) {
    updateSettingsLEDs(ledMatrix, uiState);
} else {
    // Existing LED logic
}
```

### Phase 6: Settings Persistence (Future Enhancement)

#### 6.1 EEPROM Storage
- Store voice preset selections
- Store global settings
- Auto-load on startup

#### 6.2 Settings Structure
```cpp
struct SettingsData {
    uint8_t voice1Preset;
    uint8_t voice2Preset;
    uint8_t globalVolume;
    uint8_t midiChannel;
    bool scaleLock;
    bool autoSave;
    uint32_t checksum;
};
```

## Implementation Steps

### Step 1: Core Infrastructure
1. Extend `UIState.h` with settings mode fields
2. Add preset name constants to `Voice.cpp`
3. Create settings menu helper functions

### Step 2: Button Handling
1. Modify `UIEventHandler.cpp` PLAY_STOP logic
2. Add long-press detection for settings entry
3. Implement step button handling for settings navigation

### Step 3: Display System
1. Add settings display functions to `oled.cpp`
2. Implement menu rendering
3. Add preset selection display

### Step 4: Voice Management Integration
1. Add preset application functions
2. Integrate with existing VoiceManager
3. Test preset switching

### Step 5: LED Feedback
1. Implement settings mode LED patterns
2. Add visual feedback for navigation
3. Test user experience

### Step 6: Testing & Refinement
1. Test all navigation paths
2. Verify preset changes take effect
3. Ensure smooth exit back to normal mode
4. Test edge cases and error handling

## User Experience Flow

### Entering Settings Mode
1. Ensure sequencer is stopped
2. Long-press PLAY_STOP button (>500ms)
3. OLED shows "Settings" menu
4. Step buttons 1-8 light up
5. PLAY_STOP button pulses slowly

### Navigating Settings
1. Press step buttons 1-8 to select menu items
2. Current selection highlighted on OLED
3. Press selected button again to enter/activate

### Changing Voice Presets
1. Select "Voice 1 Preset" or "Voice 2 Preset"
2. Enter preset selection submenu
3. Use step buttons 1-6 to select preset
4. Current preset marked with indicator
5. Press step button 8 to apply changes
6. Press step button 7 to go back without changes

### Exiting Settings Mode
1. Select "Exit Settings" (step button 8)
2. Or press PLAY_STOP button
3. Return to normal sequencer display
4. Settings changes are applied immediately

## Technical Considerations

### Memory Usage
- Minimal additional RAM usage
- Preset configs are generated on-demand
- Settings state uses existing UIState structure

### Performance Impact
- Settings mode only active when sequencer stopped
- No impact on audio processing
- Minimal CPU overhead for display updates

### Compatibility
- Maintains all existing functionality
- No breaking changes to current workflow
- Backward compatible with existing sequences

## Future Enhancements

### Advanced Settings
- Filter resonance global adjustment
- Envelope curve selection
- LFO waveform selection
- Delay parameters

### Preset Management
- Custom preset creation
- Preset import/export via MIDI
- Preset morphing/interpolation

### Visual Improvements
- Animated transitions
- Progress bars for adjustable parameters
- Icons for different preset types

This implementation provides a clean, intuitive interface for voice preset management while maintaining the existing workflow and ensuring excellent user experience on the small OLED display.