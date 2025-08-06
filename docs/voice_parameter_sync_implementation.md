# Voice Parameter Synchronization Implementation

## Overview

This document describes the implementation of unified state management for voice parameter toggles in the PicoMudrasSequencer, ensuring immediate synchronization between button handling, voice configuration, and OLED display updates.

## Key Issues Fixed

### 1. Voice ID Mapping Correction
**Problem**: Voice parameter toggles were using `buttonIndex - 9` to calculate voice ID, causing incorrect voice selection.

**Solution**: Modified `UIEventHandler.cpp` to use the currently selected voice based on `uiState.isVoice2Mode`:
```cpp
// OLD: uint8_t currentVoiceId = evt.buttonIndex - 9;
// NEW: 
uint8_t currentVoiceId = uiState.isVoice2Mode ? bassVoiceId : leadVoiceId;
uint8_t voiceDisplayNumber = uiState.isVoice2Mode ? 2 : 1;
```

### 2. OLED Display Real-time Updates
**Problem**: OLED display was not updating immediately when parameter toggle buttons were pressed.

**Solution**: Implemented observer pattern and immediate update callbacks:
- Added `VoiceParameterObserver` interface in `UIState.h`
- Enhanced `OLEDDisplay` to implement the observer interface
- Added `forceUpdate()` method for immediate display updates
- Implemented callback mechanism in `UIEventHandler`

### 3. Voice Switching Logic
**Problem**: Voice switching (button 24) didn't immediately update the OLED display.

**Solution**: Added immediate OLED update trigger when voice switching occurs:
```cpp
// Trigger immediate OLED update to show voice switch if in settings mode
if (uiState.settingsMode && oledUpdateCallback) {
  uiState.voiceParameterChanged = true; // Force display update
  oledUpdateCallback(uiState, voiceManager.get());
}
```

## Architecture Components

### 1. Enhanced UIState (`src/ui/UIState.h`)
- Added `VoiceParameterObserver` interface
- Added notification system with `notifyVoiceParameterChanged()`
- Added state tracking flags: `voiceParameterChanged`, `changedVoiceId`, `changedParameterName`
- Added observer registration: `voiceParameterObserver`

### 2. Updated UIEventHandler (`src/ui/UIEventHandler.cpp`)
- Fixed voice ID calculation to use `leadVoiceId`/`bassVoiceId`
- Added immediate OLED update callbacks
- Enhanced debug output for state synchronization verification
- Added voice switching OLED update trigger

### 3. Enhanced OLED Display (`src/OLED/oled.h`, `src/OLED/oled.cpp`)
- Implemented `VoiceParameterObserver` interface
- Added `forceUpdate()` method for immediate updates
- Enhanced `displayVoiceParameterToggles()` to show current voice selection
- Added visual indicators for currently active voice

### 4. Main Loop Integration (`PicoMudrasSequencer.ino`)
- Registered OLED display as voice parameter observer
- Set up callback for immediate OLED updates
- Added flag clearing after display updates

## State Synchronization Flow

1. **Button Press**: User presses parameter toggle button (9-14)
2. **Voice Selection**: UIEventHandler determines current voice using `uiState.isVoice2Mode`
3. **Parameter Update**: Voice configuration is modified via `VoiceManager`
4. **State Notification**: `uiState.notifyVoiceParameterChanged()` is called
5. **Immediate Callback**: OLED update callback is triggered immediately
6. **Display Update**: OLED shows updated parameter states with visual feedback
7. **Flag Clearing**: Main loop clears change flags after display update

## Testing Instructions

### 1. Voice Parameter Toggle Test
1. Enter settings mode (stop sequencer with button 26)
2. Switch between Voice 1 and Voice 2 using button 24
3. Press parameter toggle buttons (9-14) and verify:
   - Serial output shows correct voice number
   - OLED immediately updates to show parameter changes
   - Visual indicator shows currently selected voice

### 2. Voice Switching Test
1. In settings mode, press button 24 to switch voices
2. Verify OLED header shows "V1 ACTIVE" or "V2 ACTIVE"
3. Verify voice selection indicator (">V1" or ">V2") updates immediately

### 3. State Consistency Test
1. Toggle parameters on Voice 1, then switch to Voice 2
2. Verify Voice 2 shows different parameter states
3. Switch back to Voice 1 and verify original states are preserved

## Debug Output

The implementation includes comprehensive debug output:

```
Matrix event: button 10 pressed
Voice 2 overdrive ON
State Sync Debug - VoiceID: 1, Button: 10, UIState.voiceParameterChanged: 1, UIState.isVoice2Mode: 1
Triggering immediate OLED update via callback
OLED: Force update called - settingsMode: 1, voiceParameterChanged: 1, inVoiceParameterMode: 1
OLED: Conditions met - updating display
OLED: Force update completed - displaying voice parameter toggles
```
