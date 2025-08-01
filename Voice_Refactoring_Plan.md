# Voice System Refactoring Plan: Scalable Multi-Voice Architecture

## Current Architecture Limitations

The current dual-voice system has several hardcoded elements that prevent easy expansion:

### Hardcoded Elements
1. **Global Variables**: `filterfreq1`, `filterfreq2`, `voiceState1`, `voiceState2`
2. **Oscillator Arrays**: `osc1A`, `osc1B`, `osc1C`, `osc2A`, `osc2B`, `osc2C`
3. **Filter Objects**: `filt1`, `filt2`
4. **Envelope Objects**: `env1`, `env2`
5. **Sequencer Objects**: `seq1`, `seq2`
6. **Conditional Logic**: `isVoice2 ? voice2_param : voice1_param`

## Proposed Refactored Architecture

### 1. Voice Class Design

```cpp
class Voice {
public:
    // Voice configuration
    struct VoiceConfig {
        uint8_t oscillatorCount = 3;
        daisysp::Oscillator::WaveForm oscWaveforms[3];
        float oscAmplitudes[3];
        float oscDetuning[3];
        
        // Filter settings
        float filterRes = 0.4f;
        float filterDrive = 1.1f;
        float filterPassbandGain = 0.23f;
        
        // High-pass settings
        float highPassFreq = 80.0f;
        
        // Effects chain
        bool hasOverdrive = false;
        bool hasWavefolder = false;
        float overdriveAmount = 0.25f;
        float wavefolderGain = 1.5f;
        
        // Envelope settings
        float defaultAttack = 0.04f;
        float defaultDecay = 0.14f;
        float defaultSustain = 0.5f;
        float defaultRelease = 0.1f;
    };
    
private:
    uint8_t voiceId;
    VoiceConfig config;
    
    // Audio components
    std::vector<daisysp::Oscillator> oscillators;
    daisysp::LadderFilter filter;
    daisysp::Svf highPassFilter;
    daisysp::Adsr envelope;
    daisysp::Overdrive overdrive;
    daisysp::Wavefolder wavefolder;
    
    // Voice state
    VoiceState state;
    float filterFrequency;
    SlewParams freqSlew[3]; // For slide functionality
    
    // Gate and timing
    volatile bool gate;
    GateTimer gateTimer;
    
public:
    Voice(uint8_t id, const VoiceConfig& cfg);
    
    // Initialization
    void init(float sampleRate);
    void setConfig(const VoiceConfig& cfg);
    
    // Audio processing
    float process();
    void updateParameters(const VoiceState& newState);
    
    // Sequencer integration
    void setSequencer(std::unique_ptr<Sequencer> seq);
    Sequencer* getSequencer() { return sequencer.get(); }
    
    // State management
    VoiceState& getState() { return state; }
    void setGate(bool gateState) { gate = gateState; }
    bool getGate() const { return gate; }
    
    // Filter control
    void setFilterFrequency(float freq) { filterFrequency = freq; }
    float getFilterFrequency() const { return filterFrequency; }
    
private:
    std::unique_ptr<Sequencer> sequencer;
    void processEffectsChain(float& signal);
    void updateOscillatorFrequencies();
};
```

### 2. Voice Manager Class

```cpp
class VoiceManager {
private:
    std::vector<std::unique_ptr<Voice>> voices;
    uint8_t maxVoices;
    
public:
    VoiceManager(uint8_t maxVoiceCount = 8) : maxVoices(maxVoiceCount) {}
    
    // Voice management
    uint8_t addVoice(const Voice::VoiceConfig& config);
    void removeVoice(uint8_t voiceId);
    Voice* getVoice(uint8_t voiceId);
    uint8_t getVoiceCount() const { return voices.size(); }
    
    // Audio processing
    float processAllVoices();
    void updateAllVoices();
    
    // Sequencer integration
    void advanceAllSequencers(uint32_t clockStep);
    void startAllSequencers();
    void stopAllSequencers();
    
    // Configuration
    void setVoiceConfig(uint8_t voiceId, const Voice::VoiceConfig& config);
    Voice::VoiceConfig getVoiceConfig(uint8_t voiceId) const;
};
```

### 3. Predefined Voice Configurations

```cpp
namespace VoicePresets {
    // Analog-style voice (current Voice 1)
    Voice::VoiceConfig getAnalogVoice() {
        Voice::VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 1.0f;
        config.oscAmplitudes[1] = 1.0f;
        config.oscAmplitudes[2] = 1.0f;
        
        config.filterRes = 0.4f;
        config.filterDrive = 1.1f;
        config.filterPassbandGain = 0.23f;
        config.highPassFreq = 80.0f;
        
        config.hasOverdrive = true;
        config.hasWavefolder = true;
        config.overdriveAmount = 0.25f;
        config.wavefolderGain = 1.5f;
        
        return config;
    }
    
    // Digital-style voice (current Voice 2)
    Voice::VoiceConfig getDigitalVoice() {
        Voice::VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 0.5f;
        config.oscAmplitudes[1] = 0.5f;
        config.oscAmplitudes[2] = 2.0f;
        
        config.filterRes = 0.22f;
        config.filterDrive = 2.0f;
        config.filterPassbandGain = 0.14f;
        config.highPassFreq = 140.0f;
        
        config.hasOverdrive = false;
        config.hasWavefolder = false;
        
        return config;
    }
    
    // Bass voice
    Voice::VoiceConfig getBassVoice() {
        Voice::VoiceConfig config;
        config.oscillatorCount = 2;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscAmplitudes[0] = 1.0f;
        config.oscAmplitudes[1] = 0.7f;
        config.oscDetuning[1] = -12.0f; // One octave down
        
        config.filterRes = 0.6f;
        config.filterDrive = 1.5f;
        config.highPassFreq = 40.0f; // Lower for bass
        
        return config;
    }
    
    // Lead voice
    Voice::VoiceConfig getLeadVoice() {
        Voice::VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscDetuning[1] = 7.0f;  // Perfect fifth
        config.oscDetuning[2] = -5.0f; // Slight detune
        
        config.filterRes = 0.8f;
        config.filterDrive = 2.5f;
        config.highPassFreq = 100.0f;
        
        config.hasOverdrive = true;
        config.overdriveAmount = 0.4f;
        
        return config;
    }
}
```

### 4. Refactored Main Audio Processing

```cpp
// Replace current hardcoded audio processing with:
void fill_audio_buffer(audio_buffer_t *buffer) {
    int N = buffer->max_sample_count;
    int16_t *out = reinterpret_cast<int16_t *>(buffer->buffer->bytes);
    
    for (int i = 0; i < N; ++i) {
        // Process LFOs
        float lfo1Output = lfo1.Process();
        float lfo2Output = lfo2.Process();
        
        // Process all voices and mix
        float mixedVoices = voiceManager.processAllVoices();
        
        // Apply global effects (delay, etc.)
        float finalOutput = processGlobalEffects(mixedVoices);
        
        // Convert to output format
        out[2 * i + 0] = convertSampleToInt16(finalOutput);
        out[2 * i + 1] = convertSampleToInt16(finalOutput);
    }
    
    buffer->sample_count = N;
}
```

### 5. Dynamic Voice Creation Example

```cpp
// In setup() or runtime:
void setupVoices() {
    // Create initial voices with presets
    uint8_t voice1Id = voiceManager.addVoice(VoicePresets::getAnalogVoice());
    uint8_t voice2Id = voiceManager.addVoice(VoicePresets::getDigitalVoice());
    
    // Optionally add more voices
    uint8_t bassId = voiceManager.addVoice(VoicePresets::getBassVoice());
    uint8_t leadId = voiceManager.addVoice(VoicePresets::getLeadVoice());
    
    // Configure sequencers for each voice
    voiceManager.getVoice(voice1Id)->setSequencer(std::make_unique<Sequencer>(1));
    voiceManager.getVoice(voice2Id)->setSequencer(std::make_unique<Sequencer>(2));
    voiceManager.getVoice(bassId)->setSequencer(std::make_unique<Sequencer>(3));
    voiceManager.getVoice(leadId)->setSequencer(std::make_unique<Sequencer>(4));
}
```

## Implementation Strategy

### Phase 1: Create Voice Class
1. Implement the `Voice` class with current Voice 1 functionality
2. Test with single voice to ensure audio quality matches current implementation
3. Add configuration system for different voice types

### Phase 2: Implement Voice Manager
1. Create `VoiceManager` class
2. Refactor current dual-voice system to use Voice Manager with 2 voices
3. Ensure backward compatibility with existing functionality

### Phase 3: Add Voice Presets
1. Create preset system with current voice configurations
2. Add new voice types (bass, lead, pad, etc.)
3. Implement runtime voice configuration changes

### Phase 4: UI Integration
1. Extend UI to support voice selection and switching
2. Add voice configuration menus
3. Implement voice-specific parameter editing

### Phase 5: Advanced Features
1. Voice layering and splitting
2. Per-voice effects chains
3. Voice grouping and routing
4. Save/load voice configurations

## Benefits of This Architecture

1. **Scalability**: Easy to add Voice 3, 4, 5... up to hardware limits
2. **Modularity**: Each voice is self-contained and configurable
3. **Flexibility**: Different voice types with unique characteristics
4. **Maintainability**: Cleaner code structure, easier debugging
5. **Extensibility**: Easy to add new voice types and effects
6. **Memory Efficiency**: Only allocate resources for active voices

## Memory Considerations

For Raspberry Pi Pico (264KB RAM):
- Each voice uses approximately 15-20KB RAM
- Theoretical maximum: ~10-12 voices
- Practical maximum: 6-8 voices (leaving room for other systems)
- Use voice pooling for dynamic allocation

## Migration Path

The refactoring can be done incrementally:
1. Keep current system working
2. Implement new Voice class alongside existing code
3. Gradually migrate functionality
4. Remove old hardcoded elements once new system is stable

This approach ensures you can always fall back to the working system during development.