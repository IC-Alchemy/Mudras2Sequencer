#pragma once

#include "Voice.h"
#include "../dsp/oscillator.h"
#include "../dsp/ladder.h"
#include "../dsp/svf.h"
#include "../dsp/adsr.h"
#include "../dsp/particle.h"
#include "../dsp/overdrive.h"
#include "../dsp/wavefolder.h"
#include "../dsp/tremolo.h"
#include "../dsp/vosim.h"
#include "../dsp/whitenoise.h"
#include "../sequencer/SequencerDefs.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

/**
 * @brief Abstract base class for all audio processors in the modular voice system
 */
class AudioProcessor {
public:
    virtual ~AudioProcessor() = default;
    virtual void init(float sampleRate) = 0;
    virtual float process(float input) = 0;
    virtual void setParameter(const std::string& name, float value) = 0;
    virtual bool getParameter(const std::string& name, float& value) const = 0;
    virtual void reset() = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual std::string getType() const = 0;
};

/**
 * @brief Base class for audio sources (oscillators, noise generators, etc.)
 */
class AudioSource : public AudioProcessor {
public:
    virtual float process() = 0; // Sources don't take input
    float process(float input) override { (void)input; return process(); }
    virtual void setFrequency(float freq) = 0;
    virtual void setAmplitude(float amp) = 0;
};

/**
 * @brief Base class for audio filters
 */
class AudioFilter : public AudioProcessor {
public:
    virtual void setFrequency(float freq) = 0;
    virtual void setResonance(float res) = 0;
};

/**
 * @brief Base class for audio effects
 */
class AudioEffect : public AudioProcessor {
public:
    virtual void setMix(float mix) = 0;
};

/**
 * @brief Base class for envelopes
 */
class AudioEnvelope : public AudioProcessor {
public:
    virtual void trigger(bool gate) = 0;
    virtual bool isActive() const = 0;
    virtual void retrigger() = 0;
};

/**
 * @brief Signal chain for processing audio through multiple processors
 */
class SignalChain {
private:
    std::vector<std::unique_ptr<AudioProcessor>> processors;
    std::vector<bool> bypassStates;
    float sampleRate;
    
public:
    void init(float sr);
    void addProcessor(std::unique_ptr<AudioProcessor> processor);
    void removeProcessor(size_t index);
    void moveProcessor(size_t from, size_t to);
    void setBypass(size_t index, bool bypass);
    float process(float input);
    void reset();
    size_t getProcessorCount() const { return processors.size(); }
    
    // Parameter routing
    void setProcessorParameter(size_t index, const std::string& param, float value);
    bool getProcessorParameter(size_t index, const std::string& param, float& value) const;
    AudioProcessor* getProcessor(size_t index) const;
};

/**
 * @brief Enhanced configuration for modular voices
 */
struct ModularVoiceConfig {
    // Backward compatibility with legacy VoiceConfig
    VoiceConfig legacyConfig;
    
    // New modular configuration
    struct SourceConfig {
        std::string type; // "oscillator", "particle", "noise", "vosim"
        std::map<std::string, float> parameters;
        bool enabled = true;
    };
    
    struct ProcessorConfig {
        std::string type;
        std::map<std::string, float> parameters;
        bool enabled = true;
        size_t position = 0; // Position in signal chain
    };
    
    std::vector<SourceConfig> sources;
    std::vector<ProcessorConfig> filters;
    std::vector<ProcessorConfig> effects;
    std::vector<ProcessorConfig> envelopes;
    
    // Signal routing and mixing
    struct RoutingConfig {
        std::vector<std::pair<size_t, size_t>> connections; // (from, to) indices
        float sourceMixLevels[8] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Mix levels for up to 8 sources
    } routing;
    
    // Global voice settings
    float outputLevel = 1.0f;
    bool enabled = true;
    bool useModularMode = false; // If false, use legacy mode
    
    // Conversion functions for backward compatibility
    static ModularVoiceConfig fromLegacy(const VoiceConfig& legacy);
    VoiceConfig toLegacy() const;
};

/**
 * @brief Factory for creating audio processors
 */
class AudioProcessorFactory {
public:
    // Source creation
    static std::unique_ptr<AudioSource> createOscillator(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioSource> createParticle(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioSource> createNoise(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioSource> createVosim(const std::map<std::string, float>& params);
    
    // Filter creation
    static std::unique_ptr<AudioFilter> createLadderFilter(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioFilter> createSvfFilter(const std::map<std::string, float>& params);
    
    // Effect creation
    static std::unique_ptr<AudioEffect> createOverdrive(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioEffect> createWavefolder(const std::map<std::string, float>& params);
    static std::unique_ptr<AudioEffect> createTremolo(const std::map<std::string, float>& params);
    
    // Envelope creation
    static std::unique_ptr<AudioEnvelope> createAdsr(const std::map<std::string, float>& params);
    
    // Generic creation
    static std::unique_ptr<AudioProcessor> createProcessor(const std::string& type, const std::map<std::string, float>& params);
};


/**
 * @brief Parameter mapper for compatibility with existing parameter system
 */
class ModularParameterMapper {
public:
    // Maps ParameterManager values to modular voice parameters
    static void mapParameters(const VoiceState& state, class ModularVoice& voice);
    
    // Backward compatibility mapping
    static void mapLegacyParameters(const VoiceState& state, Voice& voice);
    
    // Real-time parameter updates
    static void updateParameter(ParamId id, float value, class ModularVoice& voice);
};

/**
 * @brief Modular voice implementation with backward compatibility
 */
class ModularVoice {
private:
    uint8_t voiceId;
    ModularVoiceConfig config;
    float sampleRate;
    
    // Signal chains
    std::vector<std::unique_ptr<AudioSource>> sources;
    SignalChain filterChain;
    SignalChain effectsChain;
    std::vector<std::unique_ptr<AudioEnvelope>> envelopes;
    
    // Mixing and routing
    float sourceMix[8];
    float finalOutput;
    
    // State management
    VoiceState state;
    bool gate;
    
    // Legacy compatibility
    std::unique_ptr<Voice> legacyVoice; // For fallback
    
    // Private helper methods
    void processModularMode();
    void processLegacyMode();
    void updateSourceParameters();
    void updateFilterParameters();
    void updateEffectParameters();
    void updateEnvelopeParameters();
    
public:
    ModularVoice(uint8_t id, const ModularVoiceConfig& cfg);
    ~ModularVoice() = default;
    
    // Core audio processing
    void init(float sampleRate);
    float process();
    
    // Configuration management
    void setConfig(const ModularVoiceConfig& config);
    const ModularVoiceConfig& getConfig() const { return config; }
    ModularVoiceConfig& getConfig() { return config; }
    
    // Parameter updates (maintains compatibility)
    void updateParameters(const VoiceState& newState);
    
    // Legacy compatibility methods
    void setLegacyConfig(const VoiceConfig& config);
    VoiceConfig getLegacyConfig() const;
    
    // State management
    VoiceState& getState() { return state; }
    const VoiceState& getState() const { return state; }
    void setGate(bool gateState) { gate = gateState; }
    bool getGate() const { return gate; }
    
    // Voice identification
    uint8_t getId() const { return voiceId; }
    bool isEnabled() const { return config.enabled; }
    void setEnabled(bool enabled) { config.enabled = enabled; }
    
    // Dynamic reconfiguration
    void addSource(const std::string& type, const std::map<std::string, float>& params);
    void addFilter(const std::string& type, const std::map<std::string, float>& params);
    void addEffect(const std::string& type, const std::map<std::string, float>& params);
    void removeProcessor(const std::string& chainType, size_t index);
    
    // Real-time parameter control
    void setSourceParameter(size_t sourceIndex, const std::string& param, float value);
    void setFilterParameter(size_t filterIndex, const std::string& param, float value);
    void setEffectParameter(size_t effectIndex, const std::string& param, float value);
    
    // Frequency control
    void setFrequency(float frequency);
    void setSlideTime(float slideTime);
    
    // Sequencer integration
    void setSequencer(std::unique_ptr<Sequencer> seq);
    void setSequencer(Sequencer* seq);
    Sequencer* getSequencer() { return legacyVoice ? legacyVoice->getSequencer() : nullptr; }
};

/**
 * @brief Enhanced voice manager with modular support
 */
class EnhancedVoiceManager {
private:
    std::vector<std::unique_ptr<ModularVoice>> modularVoices;
    bool useModularVoices = false;
    
public:
    // Initialization
    void init(float sampleRate);
    
    // Voice management
    void createVoice(uint8_t voiceId, const ModularVoiceConfig& config);
    void createVoiceFromLegacy(uint8_t voiceId, const VoiceConfig& legacyConfig);
    
    // Backward compatible methods
    void updateVoiceState(uint8_t voiceId, const VoiceState& state);
    void setVoiceFrequency(uint8_t voiceId, float frequency);
    float processVoice(uint8_t voiceId);
    
    // New modular methods
    void enableModularMode(bool enable) { useModularVoices = enable; }
    void setVoiceConfig(uint8_t voiceId, const ModularVoiceConfig& config);
    void loadVoicePreset(uint8_t voiceId, const std::string& presetName);
    
    // Voice access
    ModularVoice* getVoice(uint8_t voiceId);
    const ModularVoice* getVoice(uint8_t voiceId) const;
    
    // Utility
    size_t getVoiceCount() const { return modularVoices.size(); }
    
    // Combined processing for all voices
    float processAllVoices();
    
    // Backward-compatible controls for slide and volume
    void setVoiceSlide(uint8_t voiceId, float slideTime);
    void setVoiceVolume(uint8_t voiceId, float volume);
};