# Documentation for **Voice** and **VoiceManager** Modules  

*Location:*  
- [`src/voice/Voice.cpp`](src/voice/Voice.cpp:1)  
- [`src/voice/VoiceManager.cpp`](src/voice/VoiceManager.cpp:1)  

---

## Table of Contents
1. [Overview](#overview)  
2. [Voice Class](#voice-class)  
   - [Constructor](#constructor)  
   - [Initialization](#initialization)  
   - [Configuration](#configuration)  
   - [Audio Processing](#audio-processing)  
   - [State & Sequencer Management](#state--sequencer-management)  
   - [DSP Helper Methods](#dsp-helper-methods)  
   - [Presets (`VoicePresets` namespace)](#presets-voicepresets-namespace)  
3. [VoiceManager Class](#voicemanager-class)  
   - [Construction & Core Data Structures](#construction--core-data-structures)  
   - [Voice Lifecycle Management](#voice-lifecycle-management)  
   - [Real‑Time Parameter Updates](#real‑time-parameter-updates)  
   - [Audio Mixing & Output Control](#audio-mixing--output-control)  
   - [Preset Helpers & Utility Functions](#preset-helpers--utility-functions)  
4. [Interaction Between Voice and VoiceManager](#interaction-between-voice-and-voicemanager)  
5. [Usage Example](#usage-example)  
6. [Key Algorithms & Design Notes](#key-algorithms--design-notes)  

---

## Overview  

The **Voice** module implements a single polyphonic voice – a self‑contained DSP chain capable of generating, filtering, and shaping a sound based on a `VoiceConfig`. It supports multiple oscillators, a particle‑engine path, over‑drive, wave‑folding, envelope handling, and per‑voice sequencer attachment.

The **VoiceManager** module maintains a collection of `Voice` objects, handling allocation, identification, real‑time state updates, and final mixing of all active voices. It also provides convenience functions for preset handling and resource‑usage inspection.

Both modules are designed for an **embedded DSP environment** (Arduino / Daisy Seed) with a fixed 48 kHz sample rate by default.

---

## Voice Class  

### Constructor  

```cpp
Voice::Voice(uint8_t id, const VoiceConfig &cfg) // [src/voice/Voice.cpp:13‑38]
```  

* **Parameters**  
  - `id` – Unique voice identifier (0‑255).  
  - `cfg` – Full configuration struct defining oscillator count, waveforms, filter settings, etc.  

* **Behaviour**  
  - Reserves a vector of `Oscillator` objects (`oscillators.resize(cfg.oscillatorCount)`).  
  - Initializes three frequency‑slew structs to 440 Hz.  
  - Sets default `VoiceState` values (note, velocity, filter, envelope, gate, slide, etc.).  

---

### Initialization  

```cpp
void Voice::init(float sr) // [src/voice/Voice.cpp:40‑105]
```  

*Configures all DSP components for the current `sampleRate` (`sr`).*  

| Component | Init Call | Key Parameters |
|-----------|----------|----------------|
| Oscillators (`oscillators[i]`) | `Init(sr)`, `SetWaveform()`, `SetAmp()`, `SetPw()` (if square) | Waveform, amplitude, pulse‑width |
| Noise Generator (`noise_`) | `Init()`, `SetSeed(1)`, `SetAmp(1.0f)` | – |
| Particle Engine (`particle_`) | `Init(sr)`, `SetFreq()`, `SetResonance()`, `SetDensity()`, `SetGain()`, `SetSpread()`, `SetSync()` | Config‑driven parameters |
| Filter (`filter`) | `Init(sr)`, `SetFreq(filterFrequency)`, `SetRes()`, `SetInputDrive()`, `SetPassbandGain()`, `SetFilterMode()` | Config‑driven |
| High‑pass filter (`highPassFilter`) | `Init(sr)`, `SetFreq()`, `SetRes()` | Config‑driven |
| Envelope (`envelope`) | `Init(sr)`, attack/decay/sustain/release from config | Config‑driven |
| Effects – Overdrive & Wavefolder | Conditional `Init()` and parameter set (`SetDrive()`, `SetGain()`, `SetOffset()`) | `config.hasOverdrive`, `config.hasWavefolder` |

---

### Configuration  

```cpp
void Voice::setConfig(const VoiceConfig &cfg) // [src/voice/Voice.cpp:107‑125]
```  

* Updates the internal `config`, resizes the oscillator vector if necessary, then re‑initializes all DSP components via `init(sampleRate)`.  

---

### Audio Processing  

```cpp
float Voice::process() // [src/voice/Voice.cpp:127‑208]
```  

*Returns the final sample value for the voice (range ≈ ‑1 .. +1).*

**Processing Steps**

1. **Gate & Envelope** – Handles optional `retrigger`, then computes `envelopeValue = envelope.Process(gate)`.  
2. **Filter Frequency Modulation** – `filter.SetFreq(100.f + filterFrequency * envelopeValue + filterFrequency * .1f)`.  
3. **Frequency Slew (Slide)** – If `state.slide` is true, updates each oscillator’s frequency using `processFrequencySlew()`.  
4. **Signal Generation** – Chooses between three paths:  
   - Particle engine (`particle_.Process()`)  
   - Noise only (percussion)  
   - Ring‑mod / Dalek mode (`hasDalek`)  
   - Normal oscillator mix (`oscillators[i].Process()`)  
5. **Effects Chain** – Calls `processEffectsChain(mixedOscillators)`.  
6. **Velocity & Output Level** – Scales by `(.25f + state.velocity)` and `config.outputLevel`.  
7. **Filtering** – Applies ladder filter → high‑pass filter.  
8. **Final Mix** – Multiplies by envelope and returns `finalOutput`.  

---

### State & Sequencer Management  

| Method | Signature | Line(s) | Description |
|--------|-----------|---------|-------------|
| `void Voice::updateParameters(const VoiceState &newState)` | [src/voice/Voice.cpp:210‑226] | Updates internal `state`, syncs gate, envelope, filter frequency, and oscillator frequencies. |
| `void Voice::setSequencer(std::unique_ptr<Sequencer> seq)` | [src/voice/Voice.cpp:228‑233] | Takes ownership of a `Sequencer` via `unique_ptr`. |
| `void Voice::setSequencer(Sequencer *seq)` | [src/voice/Voice.cpp:234‑239] | Stores a raw pointer (no ownership transfer). |
| `Sequencer* Voice::getSequencer()` *(referenced in manager)* | – | Returns the attached sequencer pointer. |

---

### DSP Helper Methods  

| Method | Signature | Line(s) | Purpose |
|--------|-----------|---------|---------|
| `void Voice::processEffectsChain(float &signal)` | [src/voice/Voice.cpp:240‑253] | Applies optional overdrive and wave‑folder effects. |
| `void Voice::updateOscillatorFrequencies()` | [src/voice/Voice.cpp:255‑316] | Calculates per‑oscillator frequencies based on `state.note`, `state.octave`, harmony intervals, and detuning; respects gate‑controlled updates and slide logic. |
| `void Voice::applyEnvelopeParameters()` | [src/voice/Voice.cpp:317‑330] | Maps `state.attack`, `state.decay` to envelope times; uses linear mapping via `daisysp::fmap`. |
| `float Voice::calculateNoteFrequency(float note, int8_t octaveOffset, int harmony)` | [src/voice/Voice.cpp:331‑351] | Converts a note (0‑47) + harmony offset to a MIDI note number and then to frequency via `daisysp::mtof`. |
| `void Voice::processFrequencySlew(uint8_t oscIndex, float targetFreq)` | [src/voice/Voice.cpp:353‑362] | Exponential slewing of oscillator frequency for smooth slides (`FREQ_SLEW_RATE`). |
| `void Voice::setFrequency(float frequency)` | [src/voice/Voice.cpp:363‑395] | Directly sets the base frequency for all active oscillators (with detuning). |
| `void Voice::setSlideTime(float slideTime)` | [src/voice/Voice.cpp:397‑404] | Placeholder – currently a no‑op (parameter suppressed). |

---

### Presets (`VoicePresets` namespace)  

All preset‑factory functions return a fully‑populated `VoiceConfig`. They are defined in the same file (`Voice.cpp`) and can be accessed via `VoiceManager::getPresetConfig()`.

| Function | Signature | Line(s) | Typical Use |
|----------|-----------|---------|-------------|
| `VoiceConfig getAnalogVoice()` | [src/voice/Voice.cpp:408‑443] | Classic analog‑style saw‑wave stack. |
| `VoiceConfig getDigitalVoice()` | [src/voice/Voice.cpp:445‑481]

# Documentation for **Voice** and **VoiceManager** Modules  

*Location:*  
- [`src/voice/Voice.cpp`](src/voice/Voice.cpp:1)  
- [`src/voice/VoiceManager.cpp`](src/voice/VoiceManager.cpp:1)  

---

## Table of Contents
1. [Overview](#overview)  
2. [Voice Class](#voice-class)  
   - [Constructor](#constructor)  
   - [Initialization](#initialization)  
   - [Configuration](#configuration)  
   - [Audio Processing](#audio-processing)  
   - [State & Sequencer Management](#state--sequencer-management)  
   - [DSP Helper Methods](#dsp-helper-methods)  
   - [Presets (`VoicePresets` namespace)](#presets-voicepresets-namespace)  
3. [VoiceManager Class](#voicemanager-class)  
   - [Construction & Core Data Structures](#construction--core-data-structures)  
   - [Voice Lifecycle Management](#voice-lifecycle-management)  
   - [Real‑Time Parameter Updates](#real‑time-parameter-updates)  
   - [Audio Mixing & Output Control](#audio-mixing--output-control)  
   - [Preset Helpers & Utility Functions](#preset-helpers--utility-functions)  
4. [Interaction Between Voice and VoiceManager](#interaction-between-voice-and-voicemanager)  
5. [Usage Example](#usage-example)  
6. [Key Algorithms & Design Notes](#key-algorithms--design-notes)  

---

## Overview  

The **Voice** module implements a single polyphonic voice – a self‑contained DSP chain capable of generating, filtering, and shaping a sound based on a `VoiceConfig`. It supports multiple oscillators, a particle‑engine path, over‑drive, wave‑folding, envelope handling, and per‑voice sequencer attachment.

The **VoiceManager** module maintains a collection of `Voice` objects, handling allocation, identification, real‑time state updates, and final mixing of all active voices. It also provides convenience functions for preset handling and resource‑usage inspection.

Both modules are designed for an **embedded DSP environment** (Arduino / Daisy Seed) with a fixed 48 kHz sample rate by default.

---

## Voice Class  

### Constructor  

```cpp
Voice::Voice(uint8_t id, const VoiceConfig &cfg) // [src/voice/Voice.cpp:13‑38]
```  

* **Parameters**  
  - `id` – Unique voice identifier (0‑255).  
  - `cfg` – Full configuration struct defining oscillator count, waveforms, filter settings, etc.  

* **Behaviour**  
  - Reserves a vector of `Oscillator` objects (`oscillators.resize(cfg.oscillatorCount)`).  
  - Initializes three frequency‑slew structs to 440 Hz.  
  - Sets default `VoiceState` values (note, velocity, filter, envelope, gate, slide, etc.).  

---

### Initialization  

```cpp
void Voice::init(float sr) // [src/voice/Voice.cpp:40‑105]
```  

*Configures all DSP components for the current `sampleRate` (`sr`).*  

| Component | Init Call | Key Parameters |
|-----------|----------|----------------|
| Oscillators (`oscillators[i]`) | `Init(sr)`, `SetWaveform()`, `SetAmp()`, `SetPw()` (if square) | Waveform, amplitude, pulse‑width |
| Noise Generator (`noise_`) | `Init()`, `SetSeed(1)`, `SetAmp(1.0f)` | – |
| Particle Engine (`particle_`) | `Init(sr)`, `SetFreq()`, `SetResonance()`, `SetDensity()`, `SetGain()`, `SetSpread()`, `SetSync()` | Config‑driven parameters |
| Filter (`filter`) | `Init(sr)`, `SetFreq(filterFrequency)`, `SetRes()`, `SetInputDrive()`, `SetPassbandGain()`, `SetFilterMode()` | Config‑driven |
| High‑pass filter (`highPassFilter`) | `Init(sr)`, `SetFreq()`, `SetRes()` | Config‑driven |
| Envelope (`envelope`) | `Init(sr)`, attack/decay/sustain/release from config | Config‑driven |
| Effects – Overdrive & Wavefolder | Conditional `Init()` and parameter set (`SetDrive()`, `SetGain()`, `SetOffset()`) | `config.hasOverdrive`, `config.hasWavefolder` |

---

### Configuration  

```cpp
void Voice::setConfig(const VoiceConfig &cfg) // [src/voice/Voice.cpp:107‑125]
```  

* Updates the internal `config`, resizes the oscillator vector if necessary, then re‑initializes all DSP components via `init(sampleRate)`.  

---

### Audio Processing  

```cpp
float Voice::process() // [src/voice/Voice.cpp:127‑208]
```  

*Returns the final sample value for the voice (range ≈ ‑1 .. +1).*

**Processing Steps**

1. **Gate & Envelope** – Handles optional `retrigger`, then computes `envelopeValue = envelope.Process(gate)`.  
2. **Filter Frequency Modulation** – `filter.SetFreq(100.f + filterFrequency * envelopeValue + filterFrequency * .1f)`.  
3. **Frequency Slew (Slide)** – If `state.slide` is true, updates each oscillator’s frequency using `processFrequencySlew()`.  
4. **Signal Generation** – Chooses between three paths:  
   - Particle engine (`particle_.Process()`)  
   - Noise only (percussion)  
   - Ring‑mod / Dalek mode (`hasDalek`)  
   - Normal oscillator mix (`oscillators[i].Process()`)  
5. **Effects Chain** – Calls `processEffectsChain(mixedOscillators)`.  
6. **Velocity & Output Level** – Scales by `(.25f + state.velocity)` and `config.outputLevel`.  
7. **Filtering** – Applies ladder filter → high‑pass filter.  
8. **Final Mix** – Multiplies by envelope and returns `finalOutput`.  

---

### State & Sequencer Management  

| Method | Signature | Line(s) | Description |
|--------|-----------|---------|-------------|
| `void Voice::updateParameters(const VoiceState &newState)` | [src/voice/Voice.cpp:210‑226] | Updates internal `state`, syncs gate, envelope, filter frequency, and oscillator frequencies. |
| `void Voice::setSequencer(std::unique_ptr<Sequencer> seq)` | [src/voice/Voice.cpp:228‑233] | Takes ownership of a `Sequencer` via `unique_ptr`. |
| `void Voice::setSequencer(Sequencer *seq)` | [src/voice/Voice.cpp:234‑239] | Stores a raw pointer (no ownership transfer). |
| `Sequencer* Voice::getSequencer()` *(referenced in manager)* | – | Returns the attached sequencer pointer. |

---

### DSP Helper Methods  

| Method | Signature | Line(s) | Purpose |
|--------|-----------|---------|---------|
| `void Voice::processEffectsChain(float &signal)` | [src/voice/Voice.cpp:240‑253] | Applies optional overdrive and wave‑folder effects. |
| `void Voice::updateOscillatorFrequencies()` | [src/voice/Voice.cpp:255‑316] | Calculates per‑oscillator frequencies based on `state.note`, `state.octave`, harmony intervals, and detuning; respects gate‑controlled updates and slide logic. |
| `void Voice::applyEnvelopeParameters()` | [src/voice/Voice.cpp:317‑330] | Maps `state.attack`, `state.decay` to envelope times; uses linear mapping via `daisysp::fmap`. |
| `float Voice::calculateNoteFrequency(float note, int8_t octaveOffset, int harmony)` | [src/voice/Voice.cpp:331‑351] | Converts a note (0‑47) + harmony offset to a MIDI note number and then to frequency via `daisysp::mtof`. |
| `void Voice::processFrequencySlew(uint8_t oscIndex, float targetFreq)` | [src/voice/Voice.cpp:353‑362] | Exponential slewing of oscillator frequency for smooth slides (`FREQ_SLEW_RATE`). |
| `void Voice::setFrequency(float frequency)` | [src/voice/Voice.cpp:363‑395] | Directly sets the base frequency for all active oscillators (with detuning). |
| `void Voice::setSlideTime(float slideTime)` | [src/voice/Voice.cpp:397‑404] | Placeholder – currently a no‑op (parameter suppressed). |

---

### Presets (`VoicePresets` namespace)  

All preset‑factory functions return a fully‑populated `VoiceConfig`. They are defined in the same file (`Voice.cpp`) and can be accessed via `VoiceManager::getPresetConfig()`.

| Function | Signature | Line(s) | Typical Use |
|----------|-----------|---------|-------------|
| `VoiceConfig getAnalogVoice()` | [src/voice/Voice.cpp:408‑443] | Classic analog‑style saw‑wave stack. |
| `VoiceConfig getDigitalVoice()` | [src/voice/Voice.cpp:445‑481] | Slightly brighter detuned saws with pulse‑width control. |
| `VoiceConfig getBassVoice()` | [src/voice/Voice.cpp:483‑514] | Two‑oscillator bass (saw + triangle). |
| `VoiceConfig getLeadVoice()` | [src/voice/Voice.cpp:516‑550] | Three saws with modest detuning for lead lines. |
| `VoiceConfig getPadVoice()` | [src/voice/Voice.cpp:552‑584] | Wide pad using harmonic intervals (root, 5th, major 3rd). |
| `VoiceConfig getPercussionVoice()` | [src/voice/Voice.cpp:586‑622] | Noise‑only voice plus overdrive/wave‑folder for percussive timbres. |
| `VoiceConfig getParticleVoice()` | [src/voice/Voice.cpp:624‑653] | Uses the particle synthesis engine for texture‑rich sounds. |
| *Metadata* – preset names & count: `VOICE_PRESET_NAMES[]` (lines 655‑661) and `VOICE_PRESET_COUNT` (line 659).  

Helper `getPresetName(uint8_t)` and `getPresetConfig(uint8_t)` (lines 662‑691) map indices to the above factories.

---

## VoiceManager Class  

### Construction & Core Data Structures  

```cpp
VoiceManager::VoiceManager(uint8_t maxVoices) // [src/voice/VoiceManager.cpp:14‑18]
```  

*Pre‑allocates a `std::vector<std::unique_ptr<ManagedVoice>> voices` with `reserve(maxVoices)`.  
`ManagedVoice` (inner struct) stores:*
- `uint8_t id;`  
- `std::unique_ptr<Voice> voice;`  
- `bool enabled = true;`  
- `float mixLevel = 1.0f;`  
- `uint8_t outputChannel = 0;`  

---

### Voice Lifecycle Management  

| Method | Signature | Line(s) | Description |
|--------|-----------|---------|-------------|
| `uint8_t addVoice(const VoiceConfig &config)` | [src/voice/VoiceManager.cpp:30‑46] | Checks capacity, generates a unique ID, creates a `Voice`, initializes it, stores in `voices`. |
| `uint8_t addVoice(const std::string &presetName)` | [src/voice/VoiceManager.cpp:57‑60] | Looks up preset via `getPresetConfig()` and forwards to the above. |
| `bool removeVoice(uint8_t voiceId)` | [src/voice/VoiceManager.cpp:72‑87] | Finds the matching `ManagedVoice` and erases it. |
| `void removeAllVoices()` | [src/voice/VoiceManager.cpp:96‑100] | Clears the vector, resetting the manager. |
| `bool setVoiceConfig(uint8_t voiceId, const VoiceConfig &config)` | [src/voice/VoiceManager.cpp:112‑121] | Calls `Voice::setConfig`. |
| `bool setVoicePreset(uint8_t voiceId, const std::string &presetName)` | [src/voice/VoiceManager.cpp:133‑140] | Wrapper that fetches a preset and updates the voice. |
| `VoiceConfig* getVoiceConfig(uint8_t voiceId)` | [src/voice/VoiceManager.cpp:151‑158] | Returns a pointer to the voice’s current config (read‑only). |
| `bool attachSequencer(uint8_t voiceId, std::unique_ptr<Sequencer> seq)` | [src/voice/VoiceManager.cpp:212‑219] | Transfers ownership of a sequencer to the voice. |
| `bool attachSequencer(uint8_t voiceId, Sequencer *seq)` | [src/voice/VoiceManager.cpp:232‑241] | Attaches a raw‑pointer sequencer (no ownership). |
| `Sequencer* getSequencer(uint8_t voiceId)` | [src/voice/VoiceManager.cpp:252‑259] | Retrieves the attached sequencer, if any. |

---

### Real‑Time Parameter Updates  

| Method | Signature | Line(s) | Purpose |
|--------|-----------|---------|---------|
| `bool updateVoiceState(uint8_t voiceId, const VoiceState &state)` | [src/voice/VoiceManager.cpp:171‑180] | Propagates live MIDI‑style state to the voice (`updateParameters`). |
| `VoiceState* getVoiceState(uint8_t voiceId)` | [src/voice/VoiceManager.cpp:193‑199] | Returns a pointer to the voice’s current state for UI/visualization. |
| `void setVoiceMix(uint8_t voiceId, float mix)` | [src/voice/VoiceManager.cpp:429‑437] | Adjusts per‑voice gain (clamped 0‑1). |
| `float getVoiceMix(uint8_t voiceId) const` | [src/voice/VoiceManager.cpp:447‑450] | Retrieves per‑voice mix. |
| `void setVoiceVolume(uint8_t voiceId, float volume)` | [src/voice/VoiceManager.cpp:648‑656] | Alias for `setVoiceMix`. |
| `void setVoiceFrequency(uint8_t voiceId, float frequency)` | [src/voice/VoiceManager.cpp:668‑674] | Directly changes base pitch. |
| `void setVoiceSlide(uint8_t voiceId, float slideTime)` | [src/voice/VoiceManager.cpp:686‑692] | Calls the voice’s `setSlideTime` (currently a placeholder). |
| `void enableVoice(uint8_t voiceId, bool enabled)` / `disableVoice(uint8_t)` | [src/voice/VoiceManager.cpp:331‑351] | Mutes/unmutes a voice without removal. |
| `bool isVoiceEnabled(uint8_t voiceId) const` | [src/voice/VoiceManager.cpp:362‑365] | Query enable state. |
| `std::vector<uint8_t> getActiveVoiceIds() const` | [src/voice/VoiceManager.cpp:375‑387] | Returns IDs of all enabled voices. |

---

### Audio Mixing & Output Control  

| Method | Signature | Line(s) | Function |
|--------|-----------|---------|----------|
| `void init(float sr)` | [src/voice/VoiceManager.cpp:270‑278] | Updates manager’s `sampleRate` and re‑initializes every voice. |
| `float processAllVoices()` | [src/voice/VoiceManager.cpp:290‑301] | Iterates over `voices`, sums each enabled voice’s output scaled by its `mixLevel`, then applies `globalVolume`. |
| `float processVoice(uint8_t voiceId)` | [src/voice/VoiceManager.cpp:313‑319] | Returns output for a single voice (useful for solo monitoring). |
| `void setVoiceOutput(uint8_t voiceId, uint8_t outputChannel)` | [src/voice/VoiceManager.cpp:462‑470] | Assigns a hardware output channel (e.g., stereo panning). |
| `uint8_t getVoiceOutput(uint8_t voiceId) const` | [src/voice/VoiceManager.cpp:480‑483] | Retrieves the assigned channel. |
| `size_t getMemoryUsage() const` | [src/voice/VoiceManager.cpp:398‑416] | Computes an approximate byte count for the manager and all contained voices/DSP objects. |

---

### Preset Helpers & Utility Functions  

| Method | Signature | Line(s) |
|--------|-----------|---------|
| `static std::vector<std::string> getAvailablePresets()` | [src/voice/VoiceManager.cpp:494‑505] | Returns the list `{"analog","digital","bass","lead","pad","percussion","particle"}`. |
| `static VoiceConfig getPresetConfig(const std::string &presetName)` | [src/voice/VoiceManager.cpp:517‑536] | Dispatches to the corresponding `VoicePresets` factory. |
| Private helpers (`findVoice`, `generateVoiceId`, callback notifiers) | Lines 548‑636 | Used internally for fast ID look‑ups, unique ID generation, and UI‑callback notifications. |

---

## Interaction Between Voice and VoiceManager  

1. **Creation** – `VoiceManager::addVoice()` constructs a `Voice` with a unique ID and immediately calls `Voice::init(sampleRate)`.  
2. **Real‑time Control** – UI or MIDI handlers call `VoiceManager::updateVoiceState()` which forwards the `VoiceState` to the underlying `Voice`. This updates gate, envelope, filter frequency, and oscillator pitch.  
3. **Processing Loop** – In the main audio callback, the application calls `VoiceManager::processAllVoices()`. The manager iterates through its `ManagedVoice` vector, invoking `Voice::process()` on each enabled voice and mixing the results.  
4. **Preset Changes** – `VoiceManager::setVoicePreset()` fetches a preset `VoiceConfig` and calls `Voice::setConfig()`, which re‑initializes the voice's DSP chain.  
5. **Sequencer Attachment** – Either overload of `attachSequencer()` stores a sequencer pointer inside the `Voice`. The voice may poll its sequencer (not shown in this file) during `process()` to drive melodic/arpeggiated patterns.  

---

## Usage Example  

Below is a minimal example showing typical manager usage inside a sketch (e.g., `PicoMudrasSequencer.ino`):

```cpp
#include "VoiceManager.h"
#include "Voice.h"

VoiceManager vm(8);               // Allow up to 8 simultaneous voices

void setup() {
    vm.init(48000.0f);            // Set sample rate
    uint8_t voiceId = vm.addVoice("lead"); // Create a lead preset voice
    VoiceState st = {};           // Zero‑initialize
    st.note = 60;                 // Middle C (MIDI note number)
    st.velocity = 0.9f;
    st.gate = true;              // Note‑on
    vm.updateVoiceState(voiceId, st);
}

float audioCallback() {
    // Called each audio frame
    return vm.processAllVoices();   // Mix all voices and output
}
```

*Key points demonstrated:* manager construction, initialization, adding a voice via preset name, setting a simple `VoiceState`, and pulling the mixed audio sample.

---

## Key Algorithms & Design Notes  

| Area | Algorithmic Detail | Rationale |
|------|--------------------|-----------|
| **Frequency Slew** | `processFrequencySlew()` uses exponential interpolation: `current += (target - current) * FREQ_SLEW_RATE`. | Provides smooth portamento without expensive per‑sample calculations. |
| **Note‑to‑Frequency** | `calculateNoteFrequency()` clamps note indices, applies a harmony offset, looks up a pre‑computed `scale` table (external `scales.h`), adds 48 to center around C4, then converts via `daisysp::mtof`. | Guarantees deterministic tuning across scales and supports micro‑tonal modifications. |
| **Envelope Mapping** | `applyEnvelopeParameters()` maps normalized `state.attack`/`state.decay` to real‑time ADSR times using linear interpolation (`daisysp::fmap`). | Allows UI sliders (0‑1) to directly control envelope timing. |
| **Voice ID Generation** | `generateVoiceId()` increments a counter, wraps on overflow, and verifies uniqueness via `findVoice()`. | Guarantees non‑zero IDs even after many add/remove cycles. |
| **Memory Usage Estimation** | `getMemoryUsage()` adds up static sizes and per‑voice component counts. | Helpful for embedded systems with tight RAM budgets. |
| **Preset Fallback** | `getPresetConfig()` defaults to the analog preset if the name is unknown. | Prevents crashes from misspelled preset names. |
| **Gate‑Controlled Frequency Updates** | Both `updateOscillatorFrequencies()` and `setFrequency()` early‑exit when `state.gate` is false. | Saves CPU cycles when a voice is silent. |

---

### Closing Remarks  

The **Voice** and **VoiceManager** modules together form a lightweight, highly configurable polyphonic synth engine tailored for embedded hardware. They expose a clean API for real‑time parameter tweaking, preset management, and flexible routing, while keeping memory allocations deterministic (pre‑allocation, no heap fragmentation during audio processing). This documentation captures the public interface, internal flow, and key design decisions to aid developers in extending, integrating, or debugging the synth core.