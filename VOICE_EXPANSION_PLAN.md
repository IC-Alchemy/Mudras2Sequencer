# PicoMudrasSequencer Voice Expansion Plan
## Adding Voice 3 and Voice 4 Support

### Overview
This document outlines the comprehensive plan to extend the PicoMudrasSequencer from supporting 2 voices to 4 voices (adding Voice 3 and Voice 4).

### Current State Analysis
- **Existing Voices**: Voice 1 and Voice 2
- **Architecture**: Modular voice system with VoiceManager, individual Voice objects, and parameter management

### Phase 1: Configuration Updates

#### 1.1 SequencerDefs.h Modifications
```cpp
// Update voice count constants
constexpr uint8_t MAX_VOICES = 4;        // Changed from 2
constexpr uint8_t DEFAULT_ACTIVE_VOICES = 4;  // Changed from 2

// Memory allocation constants
constexpr size_t VOICE_BUFFER_SIZE = 256;  // Per voice
constexpr size_t TOTAL_VOICE_BUFFER_SIZE = MAX_VOICES * VOICE_BUFFER_SIZE;
```

#### 1.2 Parameter Manager Updates
- Extend parameter arrays to support 4 voices
- Add parameter mapping for Voice 3 and Voice 4


### Phase 2: Voice System Expansion

#### 2.1 VoiceManager Class Updates
```cpp
// Add new voice instances
Voice voice3;
Voice voice4;

// Update voice array management
Voice* voices[MAX_VOICES] = {&voice1, &voice2, &voice3, &voice4};
```
The following function needs to be modified so that it cycles through all 4 voices.
```cpp 

  // Handle Voice Switch (Button 24) with long press for LFO mode
  if (evt.buttonIndex == BUTTON_VOICE_SWITCH) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
    
        midiNoteManager.onModeSwitch();
        uiState.isVoice2Mode = !uiState.isVoice2Mode;
        uiState.selectedStepForEdit = -1;
        Serial.print("Switched to Voice ");
        Serial.println(uiState.isVoice2Mode ? "2" : "1");
      }
    }
    return; // Exit after handling
  
  ```

### Phase 3: Audio Processing Integration

#### 3.1 Audio Engine Updates
- **Mixer Expansion**: Extend audio mixer to handle 4 voices
- **Gain Staging**: Adjust master gain to prevent clipping with 4 voices


- **Buffer Management**: Optimize buffer sizes for 4-voice 
### Phase 4: UI and Control Integration

#### 4.1 Parameter Display Updates
- **OLED Screen**: Add pages for Voice 3 and Voice 4 preset 
- **LED Matrix**:  When either voice 3 or voice 4 is selected we will show a new page on the LED matrix where the steps now show voice 3 and voice 4 instead of 1 and 2
- **Navigation**:  Pressing the BUTTON_VOICE_SWITCH will now cycle through 4 voices.  When either voice 3 or voice 4 is selected we will show a new page on the LED matrix where the steps now show voice 3 and voice 4 instead of 1 and 2

