# Sequencer Documentation

**Purpose:** This document provides a comprehensive overview of the `Sequencer` class, its public API, internal mechanics, and its interactions with the voice subsystem and LED matrix feedback. It also includes a diagnostic guide for the known issue where voice channels 3 and 4 do not receive updates.

## Table of Contents
- [Overview](#overview)
- [Public Interface](#public-interface)
- [Internal Mechanics](#internal-mechanics)
- [Cross‑Module Interactions](#cross-module-interactions)
  - [Voice Subsystem](#voice-subsystem)
  - [LED Matrix Feedback](#led-matrix-feedback)
- [Threading & Concurrency](#threading--concurrency)
- [Error Handling & Diagnostics](#error-handling--diagnostics)
  - [Voice 3 / 4 Troubleshooting Guide](#voice‑34‑troubleshooting-guide)
- [Testing & Validation](#testing--validation)
- [Future Work & Extensions](#future-work--extensions)
- [References](#references)

## Overview
The `Sequencer` class drives per‑step parameter sequencing for up to eight voices. It runs on the main MCU loop, generates gate pulses on GPIO 12, and writes voice‑specific gate signals on GPIO 10/11. Each parameter (`Note`, `Velocity`, `Filter`, `Gate`, `Slide`, etc.) has an independent step counter, enabling polyrhythmic patterns.

**Key design concepts**
- **Per‑parameter step counters** – `currentStepPerParam[ParamId::Count]` stores the index for each parameter, allowing different step lengths (`parameterManager.getStepCount`).  
- **Gate‑controlled note programming** – `setStepParameterValue` ignores `Note` writes when the step gate ≤ 0.5 (see lines [`#L131‑L144`](../src/sequencer/Sequencer.cpp#L131)).  
- **Slide handling** – Slides update `currentNote` without retriggering the envelope; `previousStepHadSlide` tracks slide continuity.  
- **Clock integration** – External uClock provides `current_uclock_step`; the sequencer pulses the step clock (GPIO 12) each advance.  

## Public Interface
The following members are declared in `Sequencer.h` (not shown here) and implemented in `Sequencer.cpp`.

### Constructors & Destructor
```cpp
// Default constructor – initializes GPIO pins and parameters
Sequencer::Sequencer()
// Overload that selects a MIDI channel (1‑2)
Sequencer::Sequencer(uint8_t channel)
```
Implementation spans lines [`#L58‑L84`](../src/sequencer/Sequencer.cpp#L58) and [`#L82‑L104`](../src/sequencer/Sequencer.cpp#L82).

**Preconditions:** None.  
**Postconditions:** All per‑parameter step counters set to 0, GPIO pins configured, `initializeParameters()` called.

### Core Methods
| Method | Signature | Description |
|--------|-----------|-------------|
| `bool isNotePlaying() const` | `bool isNotePlaying() const` | Returns true if the envelope is triggered and not released. |
| `void initializeParameters()` | `void initializeParameters()` | Calls `parameterManager.init()`. |
| `void setParameterStepCount(ParamId id, uint8_t steps)` | `void setParameterStepCount(ParamId id, uint8_t steps)` | Forward to `parameterManager`. |
| `uint8_t getParameterStepCount(ParamId id) const` | `uint8_t getParameterStepCount(ParamId id) const` | Retrieve step count for a parameter. |
| `float getStepParameterValue(ParamId id, uint8_t stepIdx) const` | `float getStepParameterValue(ParamId id, uint8_t stepIdx) const` | Query a stored value. |
| `void setStepParameterValue(ParamId id, uint8_t stepIdx, float value)` | `void setStepParameterValue(ParamId id, uint8_t stepIdx, float value)` | Writes a value; enforces gate‑controlled note writes (see lines [`#L131‑L144`](../src/sequencer/Sequencer.cpp#L131)). |
| `void reset()` | `void reset()` | Resets all counters, stops running, calls `handleNoteOff(nullptr)`. |
| `void advanceStep(uint8_t current_uclock_step, int mm_distance, …)` | `void advanceStep(uint8_t current_uclock_step, int mm_distance, bool is_note_button_held, bool is_velocity_button_held, bool is_filter_button_held, bool is_attack_button_held, bool is_decay_button_held, bool is_octave_button_held, int current_selected_step_for_edit, VoiceState *voiceState)` | Core step‑advance logic (see lines [`#L190‑L298`](../src/sequencer/Sequencer.cpp#L190)). |
| `void processStep(uint8_t stepIdx, VoiceState *voiceState)` | `void processStep(uint8_t stepIdx, VoiceState *voiceState)` | Executes a step using either per‑parameter indices (`UINT8_MAX`) or a fixed index (preview). |
| `void startNote(uint8_t note, uint8_t velocity, uint16_t duration)` | `void startNote(uint8_t note, uint8_t velocity, uint16_t duration)` | Triggers envelope and updates gate GPIO. |
| `void handleNoteOff(VoiceState* voiceState)` | `void handleNoteOff(VoiceState* voiceState)` | Sends MIDI note‑off if callback set, releases envelope, and resets `currentNote`. |
| `void tickNoteDuration(VoiceState* voiceState)` | `void tickNoteDuration(VoiceState* voiceState)` | Decrements note‑duration timer and calls `handleNoteOff` when expired. |
| `void playStepNow(uint8_t stepIdx, VoiceState *voiceState)` | `void playStepNow(uint8_t stepIdx, VoiceState *voiceState)` | Public preview entry point; forwards to `processStep`. |
| `void toggleStep(uint8_t stepIdx)` | `void toggleStep(uint8_t stepIdx)` | Flips the gate parameter of a step. |
| `void randomizeParameters()` | `void randomizeParameters()` | Randomizes all parameters and resets a few defaults (see lines [`#L522‑L531`](../src/sequencer/Sequencer.cpp#L522)). |
| `void setMidiNoteOffCallback(void (*callback)(uint8_t, uint8_t))` | `void setMidiNoteOffCallback(void (*callback)(uint8_t, uint8_t))` | Registers a MIDI‑note‑off handler. |

**Thread‑safety:** All methods are intended to be called from the main MCU loop or ISR‑safe contexts (e.g., `advanceStep` may be called from a timer ISR). No internal mutexes are used; callers must ensure exclusive access.

## Internal Mechanics
The sequencer's operation can be split into three phases per clock tick:

1. **Clock pulse & step index update** – lines [`#L202‑L213`](../src/sequencer/Sequencer.cpp#L202) compute `currentStep` based on `ParamId::Gate` length and wrap with modulo.  
2. **Per‑parameter counters** – lines [`#L218‑L234`](../src/sequencer/Sequencer.cpp#L218) update `currentStepPerParam` for each parameter, enabling independent step lengths.  
3. **Parameter recording** – conditional block [`#L239‑L288`](../src/sequencer/Sequencer.cpp#L239) reads sensor distance, maps to parameter ranges, and writes values using `setStepParameterValue`.

### Advance Step Example
```cpp
void Sequencer::advanceStep(uint8_t current_uclock_step, int mm_distance,
                            bool is_note_button_held, bool is_velocity_button_held,
                            bool is_filter_button_held, bool is_attack_button_held,
                            bool is_decay_button_held, bool is_octave_button_held,
                            int current_selected_step_for_edit,
                            VoiceState *voiceState)
{
    // (excerpt showing clock pulse)
    digitalWrite(12, HIGH);
    digitalWrite(12, LOW);

    // (excerpt showing per‑parameter counter update)
    currentStepPerParam[i] = current_uclock_step % paramStepCount;

    // (excerpt showing real‑time parameter recording)
    if (mm_distance >= 0 && current_selected_step_for_edit == -1) {
        // map sensor → normalizedDistance → parameter value
        setStepParameterValue(pb.id, paramStepIdx, value);
    }

    // Dispatch to processStep using per‑parameter indices
    processStep(UINT8_MAX, voiceState);
}
```
The full function spans lines [`#L190‑L298`](../src/sequencer/Sequencer.cpp#L190).

### Process Step Logic
The heart of note generation resides in `processStep`. It selects the correct step index (`usePerParameterIndices` flag) and retrieves all relevant parameters:

```cpp
void Sequencer::processStep(uint8_t stepIdx, VoiceState *voiceState)
{
    bool usePerParameterIndices = (stepIdx == UINT8_MAX);

    float gateOn = getStepParameterValue(ParamId::Gate,
        usePerParameterIndices ? currentStepPerParam[static_cast<size_t>(ParamId::Gate)] : stepIdx);

    // Retrieve note, velocity, slide, etc.
    uint8_t noteStepIdx = usePerParameterIndices ? currentStepPerParam[static_cast<size_t>(ParamId::Note)] : stepIdx;
    float noteVal = getStepParameterValue(ParamId::Note, noteStepIdx);
    // …
}
```
Full implementation lines [`#L313‑L441`](../src/sequencer/Sequencer.cpp#L313).

Key behaviours:
- **Gate check** – If `gateOn` is false, the envelope is not retriggered and the note may be sustained if a slide was active (`previousStepHadSlide`).  
- **Slide handling** – When `slideVal` is non‑zero, `currentNote` is updated without calling `startNote`; the envelope timer is only reset.  
- **Voice state update** – After envelope handling, the `voiceState` fields (`filter`, `attack`, `decay`, `velocity`, `gate`, `slide`, `gateLength`) are written (lines [`#L415‑L426`](../src/sequencer/Sequencer.cpp#L415)).  

### Note Lifecycle
`startNote` (lines [`#L442‑L447`](../src/sequencer/Sequencer.cpp#L442)) sets `currentNote`, starts `noteDuration`, and triggers the envelope. `handleNoteOff` (lines [`#L453‑L475`](../src/sequencer/Sequencer.cpp#L453)) sends optional MIDI note‑off, releases the envelope, and resets `currentNote`. The duration timer is driven by `tickNoteDuration` (lines [`#L477‑L488`](../src/sequencer/Sequencer.cpp#L477)).

## Cross‑Module Interactions
### Voice Subsystem
The sequencer updates a `VoiceState` structure (see [`src/voice/VoiceReadme.md`](src/voice/VoiceReadme.md:45)). The `VoiceManager` attaches a sequencer to each voice via `Voice::setSequencer`. During `processStep` the fields of `voiceState` are populated, e.g.:

```cpp
voiceState->filter = filterVal;
voiceState->attack = attackVal;
voiceState->decay = decayVal;
voiceState->velocity = velocityVal;
voiceState->gate = gateOn;
voiceState->slide = slideVal;
voiceState->gateLength = noteDurationTicks;
if (gateOn) {
    voiceState->note = noteVal;
    voiceState->octave = octaveOffset;
}
```
(Lines [`#L418‑L434`](../src/sequencer/Sequencer.cpp#L418))

The voice code (see [`src/voice/VoiceReadme.md`](src/voice/VoiceReadme.md:45‑70)) reads these fields in `Voice::updateParameters` and drives the DSP chain.

### LED Matrix Feedback
`LEDMatrixFeedback.cpp` queries sequencer state to visualise gates, slides, and playhead positions.

```cpp
const Step& sa = a.getStep(step);
bool phA = (a.getCurrentStepForParameter(ParamId::Gate) == step && a.isRunning());
CRGB colorA = sa.gate ? theme->gateOnV1 : theme->gateOffV1;
if (a.getStepParameterValue(ParamId::Slide, step) > 0) { nblend(colorA, theme->modSlideActive, 128); }
if (phA) { colorA += theme->playheadAccent; }
```
(Lines [`#L354‑L359`](src/LEDMatrix/LEDMatrixFeedback.cpp#L354))

The same logic is duplicated for the second voice pair using the global `ledOffset` (lines [`#L368‑L371`](src/LEDMatrix/LEDMatrixFeedback.cpp#L368)). This offset reuse is the likely root cause of the missing LED updates for voices 3 / 4 (they share the same matrix region as voices 1 / 2).

The module also calls `addPolyrhythmicOverlay` (lines [`#L114‑L136`](src/LEDMatrix/LEDMatrixFeedback.cpp#L114)) to blend additional parameter colours.

## Threading & Concurrency

## Error Handling & Diagnostics
- **Return values:** Most methods are `void`; failure conditions are asserted via early `return` (e.g., gate‑blocked note writes).  
- **Logging:** Debug prints are guarded by `#ifdef DEBUG` (not shown).  

### Voice 3 / 4 Troubleshooting Guide
1. **Verify sequencer attachment** – Ensure `VoiceManager::attachSequencer` was called for voice IDs 3 and 4.  
2. **Check step counts** – `seq.getParameterStepCount(ParamId::Gate)` must be > 0 for both sequencers; otherwise `updateGateLEDs` treats the pair as inactive.  
3. **LED offset collision** – `LEDMatrixFeedback.cpp` uses a single `ledOffset` (value 24) for both voice pairs (lines [`#L368‑L371`](src/LEDMatrix/LEDMatrixFeedback.cpp#L368)). Create a distinct constant for the second pair, e.g., `constexpr int LED_OFFSET_PAIR2 = 48;`.  
4. **Callback binding** – Confirm `Sequencer::setMidiNoteOffCallback` is set for all channels; missing callbacks do not affect LED state but hide MIDI output.  
5. **Run‑time asserts** – Insert `DEBUG_ASSERT(seq.isRunning())` before rendering pair 2 to catch silent failures.  

**Suggested Fixes**  
- Add a second