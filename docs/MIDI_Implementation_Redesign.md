# MIDI Implementation Redesign

## Overview

This document describes the complete redesign of the MIDI implementation in the PicoMudrasSequencer to follow standard MIDI best practices for proper note lifecycle management. The redesign addresses critical issues with stuck notes and improves overall MIDI reliability.

## Problems Addressed

### 1. **Octave Parameter MIDI Note-Off Issue**
- **Problem**: Higher notes with octave offsets were not being turned off properly
- **Root Cause**: MIDI note-on used `finalNote = noteVal + octaveOffset`, but note-off used raw `state.note` without octave
- **Solution**: Calculate final MIDI note with octave in `updateVoiceParameters()` before passing to MidiNoteManager

### 2. **Multiple Conflicting Note-Off Mechanisms**
- **Problem**: Three different systems could trigger note-off events simultaneously
- **Solution**: Centralized all MIDI note management in `MidiNoteManager` class

### 3. **Inconsistent Note Tracking**
- **Problem**: `currentMidiNote1/2` variables used inconsistently
- **Solution**: Comprehensive `MidiNoteTracker` structure with proper state management

### 4. **Thread Safety Issues**
- **Problem**: MIDI operations across Core0/Core1 without synchronization
- **Solution**: Atomic update mechanisms in `MidiNoteManager`

## New Architecture

### MidiNoteManager Class
Central MIDI note management system with the following features:

#### Core Components
- **MidiNoteTracker**: Per-voice state tracking with timing synchronization
- **Thread-safe operations**: Atomic updates for cross-core communication
- **Gate timing integration**: Synchronized with sequencer gate length parameters
- **Comprehensive cleanup**: Multiple cleanup methods for different scenarios

#### Key Methods
```cpp
// Core note management
void noteOn(uint8_t voiceId, int8_t midiNote, uint8_t velocity, uint8_t channel, uint16_t gateDuration);
void noteOff(uint8_t voiceId);
void updateTiming(uint16_t currentTick);

// Gate synchronization
void setGateState(uint8_t voiceId, bool gateActive, uint16_t gateDuration = 0);

// Cleanup methods
void allNotesOff();
void onSequencerStop();
void onModeSwitch();
void emergencyStop();
```

### MidiNoteTracker Structure
```cpp
struct MidiNoteTracker {
    volatile int8_t activeMidiNote = -1;        // Currently playing MIDI note
    volatile uint8_t activeVelocity = 0;        // Note velocity
    volatile uint8_t activeChannel = 1;         // MIDI channel
    volatile MidiNoteState state = INACTIVE;    // Note state
    volatile bool gateActive = false;           // Gate state
    volatile uint16_t gateStartTick = 0;        // Gate timing
    volatile uint16_t gateDurationTicks = 0;    // Gate duration
    volatile uint16_t currentTick = 0;          // Current timing
    volatile bool updateInProgress = false;     // Thread safety flag
};
```

## Integration Points

### 1. **Gate Timer Integration**
- `updateVoiceParameters()` calculates final MIDI note with octave offset
- MidiNoteManager handles all note-on/off timing
- Gate timers coordinate with MIDI timing through `updateTiming()`

### 2. **Sequencer Integration**
- Legacy callback functions refactored to use MidiNoteManager
- Proper cleanup on sequencer stop/start
- Voice mode switching triggers MIDI cleanup

### 3. **Thread Safety**
- Atomic update flags prevent race conditions
- Volatile variables for cross-core communication
- Proper synchronization between Core0 (audio) and Core1 (sequencer/MIDI)

## Key Fixes

### Octave Parameter Fix
```cpp
// OLD (broken): Used raw note value without octave
int midiNote = getMidiNote(state.note);

// NEW (fixed): Calculate final note with octave offset
int finalNoteValue = static_cast<int>(state.note) + static_cast<int>(state.octave);
int midiNote = getMidiNote(static_cast<uint8_t>(finalNoteValue));
```

### Unified Note-Off Handling
```cpp
// All note-off events now go through MidiNoteManager
midiNoteManager.updateTiming(globalTickCounter); // Handles gate expiry
midiNoteManager.setGateState(voiceId, false);    // Handles immediate gate-off
midiNoteManager.noteOff(voiceId);                // Handles manual note-off
```

### Comprehensive Cleanup
```cpp
// Sequencer stop
void onClockStop() {
    midiNoteManager.onSequencerStop(); // Comprehensive MIDI cleanup
    allNotesOff();                      // Legacy sequencer cleanup
}

// Voice mode switching
void voiceModeToggle() {
    midiNoteManager.onModeSwitch();     // Clean MIDI notes before switch
    isVoice2Mode = !isVoice2Mode;
}
```

## Testing

A comprehensive test suite (`test/test_midi_implementation.cpp`) validates:
- Basic note on/off functionality
- Monophonic behavior (new notes turn off previous notes)
- Gate timing accuracy
- Dual voice independence
- Cleanup function reliability

## Benefits

1. **Eliminates Stuck Notes**: Proper note-on/off pairing prevents hanging MIDI notes
2. **Octave Support**: Higher notes with octave offsets now turn off correctly
3. **Thread Safety**: Reliable operation across dual-core architecture
4. **Comprehensive Cleanup**: Multiple cleanup mechanisms prevent edge case issues
5. **Standard MIDI Compliance**: Follows MIDI protocol best practices
6. **Maintainable Code**: Centralized MIDI logic in dedicated manager class

## Backward Compatibility

The redesign maintains backward compatibility through:
- Legacy callback functions (`sendMidiNoteOn1/2`, `sendMidiNoteOff1/2`)
- Existing `allNotesOff()` function
- Same external API for sequencer integration
- No changes to VoiceState structure (audio synthesis unaffected)

## Future Enhancements

Potential improvements for future versions:
- MIDI channel per voice configuration
- Velocity curves and scaling
- MIDI CC output for parameters
- Polyphonic mode support
- MIDI input handling
