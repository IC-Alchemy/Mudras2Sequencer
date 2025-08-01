#include "Voice.h"
#include "../scales/scales.h"
#include "../dsp/dsp.h"
#include "Arduino.h"
#include <algorithm>
#include <cmath>

// Constants
static constexpr float FREQ_SLEW_RATE = 0.00035f; // Slide speed
static constexpr float BASE_FREQ = 110.0f; // Base frequency for note calculations

Voice::Voice(uint8_t id, const VoiceConfig& cfg) 
    : voiceId(id), config(cfg), sampleRate(48000.0f), filterFrequency(1000.0f), gate(false) {
    // Initialize oscillators vector
    oscillators.resize(config.oscillatorCount);
    
    // Initialize frequency slewing
    for (int i = 0; i < 3; i++) {
        freqSlew[i].currentFreq = 440.0f;
        freqSlew[i].targetFreq = 440.0f;
    }
    
    // Initialize voice state with defaults
    state.note = 0.0f;
    state.velocity = 0.8f;
    state.filter = 0.37f;
    state.attack = 0.01f;
    state.decay = 0.01f;
    state.octave = 0;
    state.gate = false;
    state.slide = false;
    state.retrigger = false;
    state.gateLength = 24; // Default gate length
}

void Voice::init(float sr) {
    sampleRate = sr;
    
    // Initialize oscillators
    for (size_t i = 0; i < oscillators.size() && i < config.oscillatorCount; i++) {
        oscillators[i].Init(sampleRate);
        oscillators[i].SetWaveform(config.oscWaveforms[i]);
        oscillators[i].SetAmp(config.oscAmplitudes[i]);
        
        // Set pulse width for square/pulse waves
        if (config.oscWaveforms[i] == daisysp::Oscillator::WAVE_POLYBLEP_SQUARE) {
            oscillators[i].SetPw(config.oscPulseWidth[i]);
        }
    }
    
    // Initialize filter
    filter.Init(sampleRate);
    filter.SetFreq(filterFrequency);
    filter.SetRes(config.filterRes);
    filter.SetInputDrive(config.filterDrive);
    filter.SetPassbandGain(config.filterPassbandGain);
    
    // Initialize high-pass filter
    highPassFilter.Init(sampleRate);
    highPassFilter.SetFreq(config.highPassFreq);
    
    // Initialize envelope
    envelope.Init(sampleRate);
    envelope.SetAttackTime(config.defaultAttack);
    envelope.SetDecayTime(config.defaultDecay);
    envelope.SetSustainLevel(config.defaultSustain);
    envelope.SetReleaseTime(config.defaultRelease);
    
    // Initialize effects
    if (config.hasOverdrive) {
        overdrive.Init();
        overdrive.SetDrive(config.overdriveAmount);
    }
    
    if (config.hasWavefolder) {
        wavefolder.Init();
        wavefolder.SetGain(config.wavefolderGain);
        wavefolder.SetOffset(config.wavefolderOffset);
    }
}

void Voice::setConfig(const VoiceConfig& cfg) {
    config = cfg;
    
    // Resize oscillators if needed
    if (oscillators.size() != config.oscillatorCount) {
        oscillators.resize(config.oscillatorCount);
        
        // Initialize new oscillators
        for (size_t i = 0; i < oscillators.size(); i++) {
            oscillators[i].Init(sampleRate);
        }
    }
    
    // Update all components with new configuration
    init(sampleRate);
}

float Voice::process() {
    if (!config.enabled) {
        return 0.0f;
    }
    
    // Handle envelope retrigger
    if (state.retrigger) {
        envelope.Retrigger(false);
        state.retrigger = false;
    }
    
    // Process envelope
    float envelopeValue = envelope.Process(gate);
    
    float finalSignal = 0.0f;
    
    if (config.hasParticle) {
        // Note Parameter → Particle Base Frequency
        float baseFreq = calculateNoteFrequency(state.note, state.octave);
        particle.SetFreq(baseFreq);
        
        // Process particle synthesis
        float particleSignal = particle.Process();
        
        // Mix oscillators and particle based on mix level
        float mixedOscillators = 0.0f;
        for (size_t i = 0; i < oscillators.size(); i++) {
            mixedOscillators += oscillators[i].Process();
        }
        
        // Crossfade between oscillators and particle
        finalSignal = (mixedOscillators * (1.0f - config.particleMixLevel)) + 
                     (particleSignal * config.particleMixLevel);
    } else {
        // Standard oscillator processing
        for (size_t i = 0; i < oscillators.size(); i++) {
            finalSignal += oscillators[i].Process();
        }
    }
    
    // Apply effects chain
    processEffectsChain(finalSignal);
    
    // Apply main filter (uses filterFrequency calculated from filter parameter)
    float filteredSignal = filter.Process(finalSignal);
    
    // Apply high-pass filter
    highPassFilter.Process(filteredSignal);
    float highPassedSignal = highPassFilter.High();
    
    // Apply envelope to final output
    float finalOutput = highPassedSignal * envelopeValue * config.outputLevel;
    
    return finalOutput;
}

void Voice::updateParameters(const VoiceState& newState) {
    state = newState;
    gate = state.gate;
    
    // Apply envelope parameters (decay affects envelope as usual)
    applyEnvelopeParameters();
    
    // Standard filter frequency calculation for main filter
    filterFrequency = daisysp::fmap(state.filter, 150.0f, 11710.0f, daisysp::Mapping::EXP);
    
    // Update oscillator frequencies
    updateOscillatorFrequencies();
    
    // Update particle parameters if enabled
    if (config.hasParticle) {
        // Filter Parameter → Particle Resonance (0.1 to 0.95)
        float resonance = daisysp::fmap(state.filter, 0.1f, 0.95f, daisysp::Mapping::LINEAR);
        particle.SetResonance(resonance);
        
        // Attack Parameter → Particle Density (inverted: faster attack = higher density)
        // Attack range 0.0-1.0, map to density 0.2-1.0 (inverted)
        float density = daisysp::fmap(1.0f - state.attack, 0.2f, 1.0f, daisysp::Mapping::LINEAR);
        particle.SetDensity(density);
        
        // Velocity Parameter → Particle Gain
        float gain = state.velocity * config.particleGain;
        particle.SetGain(gain);
        
        // Note Parameter → Particle Base Frequency (set in process() method)
        // This will be handled in the process() method for real-time updates
    }
}

void Voice::setSequencer(std::unique_ptr<Sequencer> seq) {
    // Extract raw pointer from unique_ptr and take ownership externally
    sequencer = seq.release();
}

void Voice::setSequencer(Sequencer* seq) {
    // Store raw pointer directly (no ownership transfer)
    sequencer = seq;
}

void Voice::processEffectsChain(float& signal) {
    if (config.hasOverdrive) {
        signal = overdrive.Process(signal);
    }
    
    if (config.hasWavefolder) {
        signal = wavefolder.Process(signal);
    }
}

void Voice::updateOscillatorFrequencies() {
    // Calculate base frequency from note and octave
    float baseFreq = calculateNoteFrequency(state.note, state.octave);
    
    // Update each oscillator with detuning
    for (size_t i = 0; i < oscillators.size() && i < 3; i++) {
        float detuneMultiplier = std::pow(2.0f, config.oscDetuning[i] / 12.0f);
        float targetFreq = baseFreq * detuneMultiplier;
        
        if (state.slide) {
            // Set target for slewing
            freqSlew[i].targetFreq = targetFreq;
        } else {
            // Set frequency directly
            oscillators[i].SetFreq(targetFreq);
            freqSlew[i].currentFreq = targetFreq;
            freqSlew[i].targetFreq = targetFreq;
        }
    }
}

void Voice::applyEnvelopeParameters() {
    // Map normalized parameters to appropriate ranges
    float attack = daisysp::fmap(state.attack, 0.005f, 0.75f, daisysp::Mapping::LINEAR);
    float decay = daisysp::fmap(state.decay, 0.01f, 0.6f, daisysp::Mapping::LINEAR);
    float release = decay; // Use decay for release in this implementation
    
    envelope.SetAttackTime(attack);
    envelope.SetDecayTime(0.05f+(release * 0.5f));
    envelope.SetReleaseTime(release);
}

float Voice::calculateNoteFrequency(float note, int8_t octaveOffset) {
    // Clamp note to valid range
    int noteIndex = std::max(0, std::min(static_cast<int>(note), 47)); // 48 notes max
    
    // Get frequency from scale using the current scale index
    extern int scale[7][48];
    extern uint8_t currentScale;
    int scaleNote = scale[currentScale][noteIndex];
    
    // Use mtof() with proper MIDI note calculation (original approach)
    // Add 48 to center the scale around middle C, then add octave offset
    float midiNote = scaleNote + 48 + octaveOffset;
    
    return daisysp::mtof(midiNote);
}

void Voice::processFrequencySlew(uint8_t oscIndex, float targetFreq) {
    if (oscIndex >= 3) return;
    
    // Exponential slewing for smooth frequency transitions
    freqSlew[oscIndex].currentFreq += 
        (freqSlew[oscIndex].targetFreq - freqSlew[oscIndex].currentFreq) * FREQ_SLEW_RATE;
}

void Voice::setFrequency(float frequency) {
    // Set the base frequency for all oscillators
    for (uint8_t i = 0; i < config.oscillatorCount && i < oscillators.size(); i++) {
        float detunedFreq = frequency * (1.0f + config.oscDetuning[i] / 100.0f);
        oscillators[i].SetFreq(detunedFreq);
        freqSlew[i].targetFreq = detunedFreq;
        freqSlew[i].currentFreq = detunedFreq;
    }
}

void Voice::setSlideTime(float slideTime) {
    // Note: VoiceSlewParams doesn't have slideTime member
    // This could be implemented by storing slideTime as a class member
    // For now, this is a placeholder implementation
    (void)slideTime; // Suppress unused parameter warning
}

// Voice Presets Implementation
namespace VoicePresets {
    VoiceConfig getAnalogVoice() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = .33f;
        config.oscAmplitudes[1] = .33f;
        config.oscAmplitudes[2] = .33f;
        config.oscDetuning[0] = 0.0f;
        config.oscDetuning[1] = 0.001f; // Slight detune
        config.oscDetuning[2] = -0.001f; // Slight detune opposite
        
        config.filterRes = 0.33f;
        config.filterDrive = 2.1f;
        config.filterPassbandGain = 0.23f;
        config.highPassFreq = 140.0f;
        
        config.hasOverdrive = false;
        config.hasWavefolder = false;
        config.overdriveAmount = 0.25f;
        config.wavefolderGain = 1.5f;
        config.wavefolderOffset = 1.0f;
        
        config.defaultAttack = 0.04f;
        config.defaultDecay = 0.14f;
        config.defaultSustain = 0.5f;
        config.defaultRelease = 0.1f;
        
        return config;
    }
    
    VoiceConfig getDigitalVoice() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 0.3f;
        config.oscAmplitudes[1] = 0.3f;
        config.oscAmplitudes[2] = .6f;
        config.oscPulseWidth[0] = 0.6f;
        config.oscPulseWidth[1] = 0.35f;
          config.oscDetuning[0] = 0.0f;
        config.oscDetuning[2] = 12.0f;
        config.oscPulseWidth[1] = 0.3f; // Narrow pulse for bass
        config.filterRes = 0.22f;
        config.filterDrive = 3.0f;
        config.filterPassbandGain = 0.24f;
        config.highPassFreq = 140.0f;
        
        config.hasOverdrive = false;
        config.hasWavefolder = false;
        
        config.defaultAttack = 0.015f;
        config.defaultDecay = 0.1f;
        config.defaultSustain = 0.4f;
        config.defaultRelease = 0.1f;
        
        return config;
    }
    
    VoiceConfig getBassVoice() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_TRI;
        config.oscAmplitudes[0] = .4f;
        config.oscAmplitudes[1] = 1.f;
        config.oscDetuning[2] = -12.0f; // One octave down
        
        config.filterRes = 0.4f;
        config.filterDrive = 2.f;
        config.filterPassbandGain = 0.22f;
        config.highPassFreq = 40.0f; // Lower for bass
        
        config.hasOverdrive = false;
        config.overdriveAmount = 0.15f; // Subtle overdrive
        
        config.defaultAttack = 0.01f;
        config.defaultDecay = 0.3f;
        config.defaultSustain = 0.7f;
        config.defaultRelease = 0.2f;
        
        return config;
    }
    
    VoiceConfig getLeadVoice() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = .55f;
        config.oscAmplitudes[1] = .33f;
        config.oscAmplitudes[2] = 0.33f;
        config.oscDetuning[0] = 0.0f;
        config.oscDetuning[1] = 0.0002f;  // Perfect fifth
        config.oscDetuning[2] = 0.0002f; // Slight detune
        
        config.filterRes = 0.45f;
        config.filterDrive =3.5f;
        config.filterPassbandGain = 0.25f;
        config.highPassFreq = 120.0f;
        
        config.hasOverdrive = true;
        config.hasWavefolder = false;
        config.overdriveAmount = 0.2f;
        config.wavefolderGain = 1.0f;
        
        config.defaultAttack = 0.02f;
        config.defaultDecay = 0.2f;
        config.defaultSustain = 0.5f;
        config.defaultRelease = 0.15f;
        
        return config;
    }
    
    VoiceConfig getPadVoice() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 0.4f;
        config.oscAmplitudes[1] = 0.4f;
        config.oscAmplitudes[2] = 0.4f;
        config.oscDetuning[0] = 0.0f;
        config.oscDetuning[1] = 12.0f;  // One octave up
        config.oscDetuning[2] = 7.0f;  // Perfect fifth above octave
        
        config.filterRes = 0.15f;
        config.filterDrive = 0.8f;
        config.filterPassbandGain = 0.13f;
        config.highPassFreq = 160.0f;
        
        config.hasOverdrive = false;
        config.hasWavefolder = false;
        
        config.defaultAttack = 0.5f;  // Slow attack for pad
        config.defaultDecay = 0.8f;
        config.defaultSustain = 0.8f;
        config.defaultRelease = 1.0f; // Long release
        
       // config.outputLevel = 0.6f; // Lower level for pad
        
        return config;
    }
    
    VoiceConfig getPercussionVoice() {
        VoiceConfig config;
        config.oscillatorCount = 2;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscAmplitudes[0] = .5f;
        config.oscAmplitudes[1] = .5f;
        config.oscDetuning[0] = 0.0f;
        config.oscDetuning[1] = 7.0f; // Fifth for metallic sound
        config.oscPulseWidth[0] = 0.1f; // Very narrow pulse
        
        config.filterRes = 0.55f; // High resonance
        config.filterDrive = 3.0f;
        config.filterPassbandGain = 0.33f;
        config.highPassFreq = 144.0f; // High-pass for percussion
        
        config.hasOverdrive = true;
        config.hasWavefolder = false;
        config.overdriveAmount = 0.2f;
        config.wavefolderGain = 3.0f;
        
        config.defaultAttack = 0.001f; // Very fast attack
        config.defaultDecay = 0.05f;   // Short decay
        config.defaultSustain = 0.1f;  // Low sustain
        config.defaultRelease = 0.1f;  // Short release
        
        return config;
    }

    // Preset name mapping for settings menu
    static const char* VOICE_PRESET_NAMES[] = {
        "Analog",
        "Digital", 
        "Bass",
        "Lead",
        "Pad",
        "Percussion"
    };

    static const uint8_t VOICE_PRESET_COUNT = 6;

    const char* getPresetName(uint8_t presetIndex) {
        if (presetIndex < VOICE_PRESET_COUNT) {
            return VOICE_PRESET_NAMES[presetIndex];
        }
        return "Unknown";
    }

    VoiceConfig getPresetConfig(uint8_t presetIndex) {
        switch(presetIndex) {
            case 0: return getAnalogVoice();
            case 1: return getDigitalVoice();
            case 2: return getBassVoice();
            case 3: return getLeadVoice();
            case 4: return getPadVoice();
            case 5: return getPercussionVoice();
            default: return getAnalogVoice(); // Default fallback
        }
    }

    uint8_t getPresetCount() {
        return VOICE_PRESET_COUNT;
    }
}