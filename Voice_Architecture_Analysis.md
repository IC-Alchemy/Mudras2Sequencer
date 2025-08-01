# Voice Architecture Analysis: PicoMudrasSequencer

## Overview

The PicoMudrasSequencer implements a dual-voice synthesizer architecture with two independent sequencers controlling separate synthesis chains. This document analyzes how Voice 1 and Voice 2 function and explains the filter frequency cross-interference issue.

## Voice Architecture

### Voice 1 (Channel 1)
- **Oscillators**: `osc1A`, `osc1B`, `osc1C` (3 sawtooth oscillators)
- **Filter**: `filt1` (Ladder filter)
- **Envelope**: `env1` (ADSR envelope)
- **Effects**: `overdrive1` (overdrive/distortion), `wavefold1` (wavefolder)
- **High-pass Filter**: `highPass1` (80Hz cutoff - removes mud)
- **Sequencer**: `seq1` (Channel 1)
- **Voice State**: `voiceState1`
- **Filter Frequency Variable**: `filterfreq1`

### Voice 2 (Channel 2)
- **Oscillators**: `osc2A`, `osc2B`, `osc2C` (2 square waves + 1 sawtooth)
- **Filter**: `filt2` (Ladder filter)
- **Envelope**: `env2` (ADSR envelope)
- **High-pass Filter**: `highPass2` (140Hz cutoff - removes lows and mud)
- **Sequencer**: `seq2` (Channel 2)
- **Voice State**: `voiceState2`
- **Filter Frequency Variable**: `filterfreq2`

## Voice Processing Flow

### Voice 1 Processing Chain
```
Oscillators (osc1A + osc1B + osc1C) 
    ↓
Overdrive Processing (overdrive1)
    ↓
Ladder Filter (filt1) - frequency controlled by filterfreq1 * envelope
    ↓
High-pass Filter (highPass1) - 80Hz cutoff
    ↓
Envelope Amplitude Modulation
    ↓
Final Voice 1 Output
```

### Voice 2 Processing Chain
```
Oscillators (osc2A + osc2B + osc2C*2) 
    ↓
Ladder Filter (filt2) - frequency controlled by filterfreq2 * envelope
    ↓
High-pass Filter (highPass2) - 140Hz cutoff
    ↓
Envelope Amplitude Modulation
    ↓
Final Voice 2 Output
```

## Filter Frequency Management

### Current Implementation
The filter frequencies are managed through global variables:
- `filterfreq1` - Controls Voice 1's ladder filter cutoff frequency
- `filterfreq2` - Controls Voice 2's ladder filter cutoff frequency

### Filter Frequency Update Process
1. **Voice State Update**: Each sequencer updates its respective `VoiceState` structure with filter parameter values (0.0-1.0)
2. **Parameter Conversion**: The `updateVoiceParameters()` function calls `calculateFilterFrequency()` to convert normalized filter values to Hz
3. **Global Variable Assignment**: The calculated frequency is assigned to the appropriate global variable:
   ```cpp
   float &filterFreq = isVoice2 ? filterfreq2 : filterfreq1;
   filterFreq = calculateFilterFrequency(state.filter);
   ```
4. **Audio Processing**: In the audio callback, filters use these global variables:
   ```cpp
   filt1.SetFreq(filterfreq1 * current_filter_env_value1);
   filt2.SetFreq(filterfreq2 * current_filter_env_value2);
   ```

## The Cross-Voice Filter Frequency Issue

### Problem Description
When modifying Voice 2's filter frequency, it also impacts Voice 1's filter frequency, creating unwanted cross-voice interference.

### Root Cause Analysis

**The issue is NOT in the current codebase architecture.** Based on the code analysis:

1. **Separate Global Variables**: Voice 1 uses `filterfreq1` and Voice 2 uses `filterfreq2` - these are completely independent
2. **Proper Voice Discrimination**: The `updateVoiceParameters()` function correctly uses the `isVoice2` parameter to select the appropriate filter frequency variable
3. **Independent Audio Processing**: Each voice's filter is set independently in the audio callback

### Potential Causes

Since the code architecture appears correct, the cross-interference could be caused by:

1. **Shared LFO Modulation**: Both voices share the same LFO outputs (`lfo1Output`, `lfo2Output`) which could be modulating filter parameters
2. **AS5600 Magnetic Encoder Interference**: The `applyAS5600BaseValues()` function might be applying encoder values to both voices simultaneously
3. **UI State Management**: The `uiState.isVoice2Mode` flag might not be properly isolating voice parameter updates
4. **MIDI CC Cross-talk**: MIDI CC messages might be affecting both voices if channel routing is incorrect
5. **Memory Corruption**: Possible buffer overflow or pointer issues affecting global variables

### Debugging Recommendations

1. **Add Debug Logging**: Insert debug prints in `updateVoiceParameters()` to verify which voice is being updated:
   ```cpp
   Serial.print("Updating Voice ");
   Serial.print(isVoice2 ? "2" : "1");
   Serial.print(" filter freq to: ");
   Serial.println(filterFreq);
   ```

2. **Monitor Global Variables**: Add periodic logging of `filterfreq1` and `filterfreq2` values to detect unexpected changes

3. **Isolate LFO Effects**: Temporarily disable LFO modulation to see if the issue persists

4. **Test AS5600 Isolation**: Disable AS5600 encoder processing to eliminate it as a cause

5. **MIDI Channel Verification**: Ensure MIDI CC messages are properly routed to the correct voice channels

## Voice Characteristics

### Voice 1 (Analog-style)
- **Waveforms**: 3 sawtooth oscillators for rich harmonic content
- **Filter**: Lower resonance (0.4), moderate input drive (1.1)
- **Effects**: Overdrive and wavefolder for analog warmth and distortion
- **High-pass**: 80Hz cutoff for subtle low-end cleanup
- **Envelope**: Longer attack/decay times for smoother transitions

### Voice 2 (Digital-style)
- **Waveforms**: 2 square waves + 1 sawtooth for digital character
- **Filter**: Lower resonance (0.22), higher input drive (2.0)
- **Effects**: No additional effects processing
- **High-pass**: 140Hz cutoff for more aggressive low-end removal
- **Envelope**: Shorter attack/decay times for punchier response

## Conclusion

The voice architecture is well-designed with proper separation between Voice 1 and Voice 2. The filter frequency cross-interference issue is likely caused by external factors such as LFO modulation, encoder interference, or UI state management rather than fundamental architectural problems. Further debugging with targeted logging and systematic elimination of potential causes is recommended to identify the root cause.