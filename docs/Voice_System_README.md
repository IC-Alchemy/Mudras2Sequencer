# Modular Voice System for PicoMudrasSequencer

This document describes the new modular voice architecture that replaces the hardcoded dual-voice system, enabling easy addition of voice3, voice4, and beyond with different oscillators, filters, and sound generators.

## Overview

The new voice system provides:
- **Scalable Architecture**: Easily add/remove voices at runtime
- **Modular Design**: Each voice encapsulates its own audio processing chain
- **Preset System**: Pre-configured voice types for different sounds
- **Memory Efficient**: Optimized for Raspberry Pi Pico's limited resources
- **Backward Compatible**: Maintains existing functionality while adding new capabilities

## Architecture Components

### 1. Voice Class (`Voice.h/cpp`)
Encapsulates all audio processing for a single voice:
- Multiple oscillators with different waveforms
- Ladder filter with envelope modulation
- High-pass filter
- ADSR envelope
- Effects chain (overdrive, wavefolder)
- Frequency slewing for smooth transitions

### 2. VoiceManager Class (`VoiceManager.h/cpp`)
Manages multiple voices:
- Dynamic voice allocation/deallocation
- Voice mixing and routing
- Parameter updates and callbacks
- Memory management
- Preset management

### 3. Voice Presets
Pre-configured voice types:
- **Analog**: 3 sawtooth oscillators, warm filter, overdrive
- **Digital**: 2 square waves + 1 sawtooth, bright filter
- **Bass**: Heavy low-end focus, sub-oscillator
- **Lead**: Cutting lead sounds with effects
- **Pad**: Lush, atmospheric textures
- **Percussion**: Sharp, percussive sounds

## Quick Start

### Basic Usage

```cpp
#include "src/voice/VoiceManager.h"

// Create a voice manager with up to 4 voices
auto voiceManager = std::make_unique<VoiceManager>(4);

// Initialize with sample rate
voiceManager->init(48000.0f);

// Add voices using presets
uint8_t voice1 = voiceManager->addVoice("analog");
uint8_t voice2 = voiceManager->addVoice("digital");
uint8_t voice3 = voiceManager->addVoice("bass");
uint8_t voice4 = voiceManager->addVoice("lead");

// Process audio
float audioOutput = voiceManager->processAllVoices();
```

### Using the Factory Pattern

```cpp
// Create common setups easily
auto dualVoiceSetup = VoiceFactory::createDualVoiceSetup();
auto quadVoiceSetup = VoiceFactory::createQuadVoiceSetup();
auto polyphonicSetup = VoiceFactory::createPolyphonicSetup();

// Custom setup
auto customSetup = VoiceFactory::createCustomSetup(
    {"bass", "lead", "pad"}, // Voice presets
    6 // Max voices
);
```

### Using the Builder Pattern

```cpp
auto voiceManager = VoiceManagerBuilder()
    .withMaxVoices(6)
    .withVoice("analog")
    .withVoice("digital")
    .withVoice("bass")
    .withGlobalVolume(0.8f)
    .withVoiceCountCallback([](uint8_t count) {
        Serial.println("Voice count: " + String(count));
    })
    .build();
```

## Migration from Current System

### Step 1: Replace Global Variables

**Before (current system):**
```cpp
// Global voice components
daisysp::Oscillator osc1A, osc1B, osc1C;
daisysp::Oscillator osc2A, osc2B, osc2C;
daisysp::MoogLadder filt1, filt2;
float filterfreq1, filterfreq2;
VoiceState voiceState1, voiceState2;
```

**After (new system):**
```cpp
// Single voice manager handles everything
std::unique_ptr<VoiceManager> voiceManager;
```

### Step 2: Replace Audio Processing Loop

**Before:**
```cpp
void fill_audio_buffer(float* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Manual processing of each voice
        float voice1_out = processVoice1();
        float voice2_out = processVoice2();
        buffer[i] = voice1_out + voice2_out;
    }
}
```

**After:**
```cpp
void fill_audio_buffer(float* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        // Automatic processing of all voices
        buffer[i] = voiceManager->processAllVoices();
    }
}
```

### Step 3: Replace Parameter Updates

**Before:**
```cpp
void updateVoiceParameters(VoiceState& state, bool isVoice2) {
    if (isVoice2) {
        filterfreq2 = calculateFilterFrequency(state.filter);
        // Update voice 2 components...
    } else {
        filterfreq1 = calculateFilterFrequency(state.filter);
        // Update voice 1 components...
    }
}
```

**After:**
```cpp
void updateVoiceParameters(uint8_t voiceId, const VoiceState& state) {
    voiceManager->updateVoiceState(voiceId, state);
}
```

## Adding New Voices

### Runtime Voice Addition

```cpp
// Add a new voice during performance
uint8_t newVoiceId = voiceManager->addVoice("pad");

// Attach a sequencer to the new voice
auto sequencer = std::make_unique<Sequencer>(newVoiceId);
voiceManager->attachSequencer(newVoiceId, std::move(sequencer));

// Configure the voice
voiceManager->setVoiceMix(newVoiceId, 0.7f);
voiceManager->enableVoice(newVoiceId, true);
```

### Creating Custom Voice Types

```cpp
// Define a custom voice configuration
VoiceConfig customVoice;
customVoice.oscillatorCount = 2;
customVoice.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
customVoice.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
customVoice.oscDetuning[1] = -12.0f; // One octave down
customVoice.filterRes = 0.8f;
customVoice.hasOverdrive = true;
customVoice.overdriveAmount = 0.3f;

// Add the custom voice
uint8_t customVoiceId = voiceManager->addVoice(customVoice);
```

## Performance Considerations

### Memory Usage

```cpp
// Monitor memory usage
size_t memoryUsage = voiceManager->getMemoryUsage();
Serial.println("Memory usage: " + String(memoryUsage) + " bytes");

// Check available slots
if (voiceManager->hasAvailableSlots()) {
    // Safe to add more voices
}
```

### CPU Optimization

```cpp
// Disable unused voices to save CPU
voiceManager->disableVoice(voiceId);

// Process individual voices for custom mixing
float voice1Out = voiceManager->processVoice(voice1Id);
float voice2Out = voiceManager->processVoice(voice2Id);
float customMix = (voice1Out * 0.7f) + (voice2Out * 0.3f);
```

## Advanced Features

### Voice Callbacks

```cpp
// Get notified when voice count changes
voiceManager->setVoiceCountCallback([](uint8_t count) {
    updateUI(count);
    updateMIDIRouting(count);
});

// Get notified when voice parameters change
voiceManager->setVoiceUpdateCallback([](uint8_t voiceId, const VoiceState& state) {
    updateMIDICC(voiceId, state);
    updateLEDDisplay(voiceId, state);
});
```

### Voice Routing

```cpp
// Route voices to different outputs
voiceManager->setVoiceOutput(bassVoiceId, 0);  // Main output
voiceManager->setVoiceOutput(leadVoiceId, 1);  // Aux output

// Adjust individual voice levels
voiceManager->setVoiceMix(bassVoiceId, 0.8f);
voiceManager->setVoiceMix(leadVoiceId, 1.0f);
```

### Dynamic Preset Switching

```cpp
// Switch voice presets during performance
voiceManager->setVoicePreset(voiceId, "lead");

// Get available presets
auto presets = VoiceManager::getAvailablePresets();
for (const auto& preset : presets) {
    Serial.println(preset.c_str());
}
```

## Integration with Existing Code

### Sequencer Integration

```cpp
// Attach existing sequencers to voices
auto sequencer1 = std::make_unique<Sequencer>(1);
auto sequencer2 = std::make_unique<Sequencer>(2);

voiceManager->attachSequencer(voice1Id, std::move(sequencer1));
voiceManager->attachSequencer(voice2Id, std::move(sequencer2));

// Access sequencers when needed
Sequencer* seq1 = voiceManager->getSequencer(voice1Id);
if (seq1) {
    seq1->advanceStep();
}
```

### MIDI Integration

```cpp
// Update voice parameters from MIDI
void onMIDICC(uint8_t channel, uint8_t cc, uint8_t value) {
    uint8_t voiceId = channel; // Map MIDI channel to voice ID
    
    VoiceState* state = voiceManager->getVoiceState(voiceId);
    if (state) {
        switch (cc) {
            case 74: // Filter
                state->filter = value / 127.0f;
                break;
            case 73: // Attack
                state->attack = value / 127.0f;
                break;
            // ... other parameters
        }
        voiceManager->updateVoiceState(voiceId, *state);
    }
}
```

## Troubleshooting

### Common Issues

1. **Memory Errors**: Reduce `maxVoices` or simplify voice configurations
2. **Audio Dropouts**: Lower voice count or disable unused voices
3. **Parameter Not Updating**: Ensure `updateVoiceState()` is called after changes
4. **No Audio Output**: Check voice is enabled and has non-zero mix level

### Debug Information

```cpp
// Print voice system status
void printVoiceStatus() {
    Serial.println("=== Voice System Status ===");
    Serial.println("Voice count: " + String(voiceManager->getVoiceCount()));
    Serial.println("Memory usage: " + String(voiceManager->getMemoryUsage()) + " bytes");
    
    auto activeVoices = voiceManager->getActiveVoiceIds();
    for (uint8_t voiceId : activeVoices) {
        Serial.println("Voice " + String(voiceId) + ": enabled=" + 
                      String(voiceManager->isVoiceEnabled(voiceId)));
    }
}
```

## Future Enhancements

- **Voice Morphing**: Smooth transitions between presets
- **Modulation Matrix**: Flexible parameter modulation
- **Voice Stealing**: Intelligent voice allocation for polyphony
- **Preset Saving**: Store custom voice configurations
- **Performance Profiles**: Optimize for different use cases

## Conclusion

The new modular voice system provides a solid foundation for expanding the PicoMudrasSequencer beyond its current dual-voice limitation. With this architecture, you can:

- Add voice3, voice4, etc. with simple function calls
- Create unique sound combinations with different oscillator/filter configurations
- Maintain backward compatibility with existing code
- Scale the system based on available memory and CPU resources

The system is designed to grow with your musical needs while maintaining the performance characteristics required for real-time audio on the Raspberry Pi Pico.