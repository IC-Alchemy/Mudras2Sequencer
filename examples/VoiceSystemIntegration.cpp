/**
 * VoiceSystemIntegration.cpp
 * 
 * This file demonstrates how to integrate the new modular voice system
 * into the existing PicoMudrasSequencer codebase.
 * 
 * It shows:
 * 1. How to replace the current dual-voice hardcoded system
 * 2. How to add new voices dynamically
 * 3. How to migrate existing functionality
 * 4. Memory management considerations for Raspberry Pi Pico
 */

#include "../src/voice/VoiceManager.h"
#include "../src/voice/Voice.h"
#include "../src/sequencer/Sequencer.h"

// Example: Replacing the current dual-voice system
class ModernPicoMudrasSequencer {
private:
    std::unique_ptr<VoiceManager> voiceManager;
    
    // Keep existing components that don't change
    // LEDMatrix, MidiManager, AS5600Manager, etc.
    
public:
    ModernPicoMudrasSequencer() {
        // Initialize with the same dual-voice setup as current implementation
        voiceManager = VoiceFactory::createDualVoiceSetup();
        
        // Set up callbacks for voice management
        voiceManager->setVoiceCountCallback([this](uint8_t count) {
            onVoiceCountChanged(count);
        });
        
        voiceManager->setVoiceUpdateCallback([this](uint8_t voiceId, const VoiceState& state) {
            onVoiceParametersChanged(voiceId, state);
        });
    }
    
    void init() {
        // Initialize audio system
        voiceManager->init(48000.0f);
        
        // Attach sequencers to voices (maintaining current behavior)
        auto sequencer1 = std::make_unique<Sequencer>(1);
        auto sequencer2 = std::make_unique<Sequencer>(2);
        
        // Get voice IDs (should be 1 and 2 for dual setup)
        auto voiceIds = voiceManager->getActiveVoiceIds();
        if (voiceIds.size() >= 2) {
            voiceManager->attachSequencer(voiceIds[0], std::move(sequencer1));
            voiceManager->attachSequencer(voiceIds[1], std::move(sequencer2));
        }
    }
    
    // Main audio processing - replaces the current fill_audio_buffer function
    void processAudio(float* buffer, size_t bufferSize) {
        for (size_t i = 0; i < bufferSize; i++) {
            // Process all voices and mix them
            float mixedOutput = voiceManager->processAllVoices();
            
            // Apply any global effects (delay, etc.) here
            // ...
            
            buffer[i] = mixedOutput;
        }
    }
    
    // Voice management methods
    uint8_t addVoice(const std::string& presetName) {
        return voiceManager->addVoice(presetName);
    }
    
    bool removeVoice(uint8_t voiceId) {
        return voiceManager->removeVoice(voiceId);
    }
    
    void setVoicePreset(uint8_t voiceId, const std::string& preset) {
        voiceManager->setVoicePreset(voiceId, preset);
    }
    
    // Update voice parameters (replaces current updateVoiceParameters)
    void updateVoiceParameters(uint8_t voiceId, const VoiceState& state) {
        voiceManager->updateVoiceState(voiceId, state);
    }
    
    // Callbacks
    void onVoiceCountChanged(uint8_t count) {
        // Update UI, MIDI routing, etc.
        Serial.print("Voice count changed to: ");
        Serial.println(count);
    }
    
    void onVoiceParametersChanged(uint8_t voiceId, const VoiceState& state) {
        // Update MIDI CC, LED display, etc.
        // This replaces the current MIDI CC update logic
    }
    
    // Memory monitoring for embedded system
    void printMemoryUsage() {
        size_t usage = voiceManager->getMemoryUsage();
        Serial.print("Voice system memory usage: ");
        Serial.print(usage);
        Serial.println(" bytes");
    }
};

// Example: Dynamic voice creation during runtime
void demonstrateDynamicVoiceCreation() {
    // Start with a basic setup
    auto voiceManager = VoiceFactory::createDualVoiceSetup();
    
    // Add a bass voice for rhythm section
    uint8_t bassVoiceId = voiceManager->addVoice("bass");
    
    // Add a lead voice for solos
    uint8_t leadVoiceId = voiceManager->addVoice("lead");
    
    // Create custom voice configuration
    VoiceConfig customConfig;
    customConfig.oscillatorCount = 2;
    customConfig.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
    customConfig.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
    customConfig.filterRes = 0.5f;
    customConfig.hasOverdrive = true;
    
    uint8_t customVoiceId = voiceManager->addVoice(customConfig);
    
    // Configure voice mixing
    voiceManager->setVoiceMix(bassVoiceId, 0.8f);   // Bass slightly quieter
    voiceManager->setVoiceMix(leadVoiceId, 1.0f);   // Lead at full volume
    voiceManager->setVoiceMix(customVoiceId, 0.6f); // Custom voice as background
}

// Example: Migration strategy from current codebase
class MigrationHelper {
public:
    // Convert current voice state to new system
    static VoiceState convertLegacyVoiceState(
        float note, float velocity, float filter, 
        float attack, float decay, int8_t octave,
        bool gate, bool slide, uint8_t gateLength) {
        
        VoiceState state;
        state.note = note;
        state.velocity = velocity;
        state.filter = filter;
        state.attack = attack;
        state.decay = decay;
        state.octave = octave;
        state.gate = gate;
        state.slide = slide;
        state.gateLength = gateLength;
        state.retrigger = false; // New parameter
        
        return state;
    }
    
    // Create voice config that matches current Voice 1 (3 sawtooth oscillators)
    static VoiceConfig createVoice1Config() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 1.0f;
        config.oscAmplitudes[1] = 1.0f;
        config.oscAmplitudes[2] = 1.0f;
        
        // Match current filter settings
        config.filterRes = 0.4f;
        config.filterDrive = 1.1f;
        config.filterPassbandGain = 0.23f;
        config.highPassFreq = 80.0f;
        
        // Match current effects
        config.hasOverdrive = true;
        config.hasWavefolder = true;
        config.overdriveAmount = 0.25f;
        
        return config;
    }
    
    // Create voice config that matches current Voice 2 (2 square + 1 sawtooth)
    static VoiceConfig createVoice2Config() {
        VoiceConfig config;
        config.oscillatorCount = 3;
        config.oscWaveforms[0] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[1] = daisysp::Oscillator::WAVE_POLYBLEP_SQUARE;
        config.oscWaveforms[2] = daisysp::Oscillator::WAVE_POLYBLEP_SAW;
        config.oscAmplitudes[0] = 0.5f;
        config.oscAmplitudes[1] = 0.5f;
        config.oscAmplitudes[2] = 2.0f;
        config.oscPulseWidth[0] = 0.6f;
        config.oscPulseWidth[1] = 0.35f;
        
        // Match current filter settings
        config.filterRes = 0.22f;
        config.filterDrive = 2.0f;
        config.filterPassbandGain = 0.14f;
        config.highPassFreq = 140.0f;
        
        // No effects for voice 2 in current implementation
        config.hasOverdrive = false;
        config.hasWavefolder = false;
        
        return config;
    }
};

// Example: Performance considerations for Raspberry Pi Pico
class PerformanceOptimizedVoiceManager {
private:
    std::unique_ptr<VoiceManager> voiceManager;
    uint8_t maxVoicesForPico;
    
public:
    PerformanceOptimizedVoiceManager() {
        // Conservative voice count for Pico's limited RAM
        maxVoicesForPico = 4; // Adjust based on testing
        
        voiceManager = VoiceManagerBuilder()
            .withMaxVoices(maxVoicesForPico)
            .withGlobalVolume(0.8f) // Prevent clipping with multiple voices
            .build();
    }
    
    void optimizeForPico() {
        // Monitor memory usage
        size_t memUsage = voiceManager->getMemoryUsage();
        
        // If memory usage is too high, reduce voice count
        if (memUsage > 50000) { // 50KB threshold (adjust as needed)
            Serial.println("Warning: High memory usage, consider reducing voices");
        }
        
        // Disable unused voices to save CPU
        auto activeVoices = voiceManager->getActiveVoiceIds();
        for (uint8_t voiceId : activeVoices) {
            VoiceState* state = voiceManager->getVoiceState(voiceId);
            if (state && !state->gate) {
                // Voice is not playing, consider disabling temporarily
                // voiceManager->disableVoice(voiceId);
            }
        }
    }
};

// Example: Real-time voice switching
class LivePerformanceVoiceManager {
private:
    std::unique_ptr<VoiceManager> voiceManager;
    std::vector<std::string> presetBank;
    uint8_t currentPresetIndex;
    
public:
    LivePerformanceVoiceManager() {
        voiceManager = VoiceFactory::createQuadVoiceSetup();
        
        // Set up preset bank for live switching
        presetBank = {"analog", "digital", "bass", "lead", "pad", "percussion"};
        currentPresetIndex = 0;
    }
    
    void switchVoicePreset(uint8_t voiceId, bool next = true) {
        if (next) {
            currentPresetIndex = (currentPresetIndex + 1) % presetBank.size();
        } else {
            currentPresetIndex = (currentPresetIndex - 1 + presetBank.size()) % presetBank.size();
        }
        
        voiceManager->setVoicePreset(voiceId, presetBank[currentPresetIndex]);
        
        Serial.print("Voice ");
        Serial.print(voiceId);
        Serial.print(" switched to: ");
        Serial.println(presetBank[currentPresetIndex].c_str());
    }
    
    void morphBetweenPresets(uint8_t voiceId, const std::string& preset1, 
                           const std::string& preset2, float morphAmount) {
        // Advanced: Morph between two presets
        // This would require interpolating between VoiceConfig parameters
        // Implementation left as exercise for advanced users
    }
};

// Example usage in main application
void setupModernVoiceSystem() {
    // Replace the current hardcoded dual-voice system
    auto modernSequencer = std::make_unique<ModernPicoMudrasSequencer>();
    modernSequencer->init();
    
    // Demonstrate adding more voices
    uint8_t bassVoice = modernSequencer->addVoice("bass");
    uint8_t leadVoice = modernSequencer->addVoice("lead");
    
    // Show memory usage
    modernSequencer->printMemoryUsage();
    
    Serial.println("Modern voice system initialized!");
    Serial.println("You can now add voice3, voice4, etc. dynamically!");
}