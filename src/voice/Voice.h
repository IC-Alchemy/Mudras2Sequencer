#pragma once

#include "../dsp/oscillator.h"
#include "../dsp/ladder.h"
#include "../dsp/svf.h"
#include "../dsp/adsr.h"
#include "../dsp/overdrive.h"
#include "../dsp/wavefolder.h"
#include "../dsp/whitenoise.h"
#include "../sequencer/Sequencer.h"
#include "../sequencer/SequencerDefs.h"
#include <vector>
#include <memory>

/**
 * @brief Configuration structure for a voice
 * Defines the characteristics and behavior of a synthesizer voice
 */
struct VoiceConfig {
    // Custom waveform constants
    static constexpr uint8_t WAVE_NOISE = 255; // Custom noise waveform
    // Oscillator configuration
    uint8_t oscillatorCount = 3;
    uint8_t oscWaveforms[3] = {
        daisysp::Oscillator::WAVE_POLYBLEP_SAW,
        daisysp::Oscillator::WAVE_POLYBLEP_SAW,
        daisysp::Oscillator::WAVE_POLYBLEP_SAW
    };
    float oscAmplitudes[3] = {1.0f, 1.0f, 1.0f};
    float oscDetuning[3] = {0.0f, 0.0f, 0.0f}; // In semitones
    float oscPulseWidth[3] = {0.5f, 0.5f, 0.5f}; // For square/pulse waves
    int harmony[3] = {0, 0, 0}; // Harmony intervals for each oscillator (scale steps)
    // Filter settings
    float filterRes = 0.4f;
    float filterDrive = 1.1f;
    float filterPassbandGain = 0.23f;
    daisysp::LadderFilter::FilterMode filterMode =daisysp::LadderFilter::FilterMode::LP24;
    // High-pass filter settings
    float highPassFreq = 80.0f;
    
    // Effects chain configuration
    bool hasOverdrive = false;
    bool hasWavefolder = false;
    float overdriveAmount = 0.25f;
    float wavefolderGain = 1.5f;
    float wavefolderOffset = 1.0f;
    
    // Envelope default settings
    float defaultAttack = 0.04f;
    float defaultDecay = 0.14f;
    float defaultSustain = 0.5f;
    float defaultRelease = 0.1f;
    
    // Voice mixing
    float outputLevel = 1.0f;
    bool enabled = true;
};

/**
 * @brief Frequency slewing parameters for smooth slide transitions
 */
struct VoiceSlewParams {
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
};

/**
 * @brief A complete synthesizer voice with oscillators, filter, envelope, and effects
 * 
 * This class encapsulates all the audio processing components needed for a single voice,
 * making it easy to create multiple independent voices with different characteristics.
 */
class Voice {
public:
    /**
     * @brief Construct a new Voice object
     * @param id Unique identifier for this voice (0-based)
     * @param config Configuration structure defining voice characteristics
     */
    Voice(uint8_t id, const VoiceConfig& config);
    
    /**
     * @brief Destroy the Voice object
     */
    ~Voice() = default;
    
    // Initialization and configuration
    /**
     * @brief Initialize the voice with the given sample rate
     * @param sampleRate Audio sample rate in Hz
     */
    void init(float sampleRate);
    
    /**
     * @brief Update the voice configuration
     * @param config New configuration to apply
     */
    void setConfig(const VoiceConfig& config);
    
    /**
     * @brief Get the current voice configuration
     * @return const VoiceConfig& Current configuration
     */
    const VoiceConfig& getConfig() const { return config; }
    
    /**
     * @brief Get mutable reference to voice configuration
     * @return VoiceConfig& Reference to voice configuration
     */
    VoiceConfig& getConfig() { return config; }
    
    // Audio processing
    /**
     * @brief Process one sample of audio
     * @return float Processed audio sample
     */
    float process();
    
    /**
     * @brief Update voice parameters from sequencer state
     * @param newState New voice state from sequencer
     */
    void updateParameters(const VoiceState& newState);
    
    // Sequencer integration
    /**
     * @brief Set the sequencer for this voice
     * @param seq Unique pointer to sequencer object
     */
    void setSequencer(std::unique_ptr<Sequencer> seq);
    
    /**
     * @brief Set the sequencer for this voice (raw pointer, no ownership transfer)
     * @param seq Raw pointer to sequencer object
     */
    void setSequencer(Sequencer* seq);
    
    /**
     * @brief Get pointer to the voice's sequencer
     * @return Sequencer* Pointer to sequencer (nullptr if not set)
     */
    Sequencer* getSequencer() { return sequencer; }
    
    // State management
    /**
     * @brief Get reference to current voice state
     * @return VoiceState& Reference to voice state
     */
    VoiceState& getState() { return state; }
    
    /**
     * @brief Get const reference to current voice state
     * @return const VoiceState& Const reference to voice state
     */
    const VoiceState& getState() const { return state; }
    
    /**
     * @brief Set gate state for this voice
     * @param gateState True for gate on, false for gate off
     */
    void setGate(bool gateState) { gate = gateState; }
    
    /**
     * @brief Get current gate state
     * @return bool Current gate state
     */
    bool getGate() const { return gate; }
    
    // Filter control
    /**
     * @brief Set filter cutoff frequency
     * @param freq Frequency in Hz
     */
    void setFilterFrequency(float freq) { filterFrequency = freq; }
    
    /**
     * @brief Get current filter frequency
     * @return float Current filter frequency in Hz
     */
    float getFilterFrequency() const { return filterFrequency; }
    
    // Voice identification
    /**
     * @brief Get voice ID
     * @return uint8_t Voice identifier
     */
    uint8_t getId() const { return voiceId; }
    
    /**
     * @brief Check if voice is enabled
     * @return bool True if voice is enabled
     */
    bool isEnabled() const { return config.enabled; }
    
    /**
     * @brief Enable or disable the voice
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled) { config.enabled = enabled; }
    
    /**
     * @brief Set the base frequency for all oscillators
     * @param frequency Frequency in Hz
     */
    void setFrequency(float frequency);
    
    /**
     * @brief Set slide time for frequency transitions
     * @param slideTime Slide time in seconds
     */
    void setSlideTime(float slideTime);
    
private:
    // Voice identification and configuration
    uint8_t voiceId;
    VoiceConfig config;
    float sampleRate;
    
    // Audio processing components
    std::vector<daisysp::Oscillator> oscillators;
    daisysp::WhiteNoise noise_;
    daisysp::LadderFilter filter;
    daisysp::Svf highPassFilter;
    daisysp::Adsr envelope;
    daisysp::Overdrive overdrive;
    daisysp::Wavefolder wavefolder;
    
    // Voice state and control
    VoiceState state;
    float filterFrequency;
    VoiceSlewParams freqSlew[3]; // For slide functionality
    volatile bool gate;
    
    // Sequencer (non-owning pointer)
    Sequencer* sequencer;
    
    // Private helper methods
    /**
     * @brief Process the effects chain on the input signal
     * @param signal Reference to signal to process (modified in place)
     */
    void processEffectsChain(float& signal);
    
    /**
     * @brief Update oscillator frequencies based on current state
     */
    void updateOscillatorFrequencies();
    
    /**
     * @brief Apply envelope parameters to the ADSR envelope
     */
    void applyEnvelopeParameters();
    
    /**
     * @brief Calculate frequency for a given note with octave offset
     * @param note Note value (0-21 for scale array lookup)
     * @param octaveOffset Octave offset in semitones
     * @param harmony Harmony value
     * @return float Frequency in Hz
     */
    float calculateNoteFrequency(float note, int8_t octaveOffset, int harmony);
    
    /**
     * @brief Process frequency slewing for slide functionality
     * @param oscIndex Oscillator index (0-2)
     * @param targetFreq Target frequency for slewing
     */
    void processFrequencySlew(uint8_t oscIndex, float targetFreq);
};

/**
 * @brief Namespace containing predefined voice configurations
 */
namespace VoicePresets {
    /**
     * @brief Get analog-style voice configuration (current Voice 1)
     * @return VoiceConfig Analog voice configuration
     */
    VoiceConfig getAnalogVoice();
    
    /**
     * @brief Get digital-style voice configuration (current Voice 2)
     * @return VoiceConfig Digital voice configuration
     */
    VoiceConfig getDigitalVoice();
    
    /**
     * @brief Get bass voice configuration
     * @return VoiceConfig Bass voice configuration
     */
    VoiceConfig getBassVoice();
    
    /**
     * @brief Get lead voice configuration
     * @return VoiceConfig Lead voice configuration
     */
    VoiceConfig getLeadVoice();
    
    /**
     * @brief Get pad voice configuration
     * @return VoiceConfig Pad voice configuration
     */
    VoiceConfig getPadVoice();
    
    /**
     * @brief Get percussion voice configuration
     * @return VoiceConfig Percussion voice configuration
     */
    VoiceConfig getPercussionVoice();
    
    /**
     * @brief Get preset name by index
     * @param presetIndex Index of the preset (0-5)
     * @return const char* Name of the preset
     */
    const char* getPresetName(uint8_t presetIndex);
    
    /**
     * @brief Get preset configuration by index
     * @param presetIndex Index of the preset (0-5)
     * @return VoiceConfig Configuration for the preset
     */
    VoiceConfig getPresetConfig(uint8_t presetIndex);
    
    /**
     * @brief Get total number of available presets
     * @return uint8_t Number of presets
     */
    uint8_t getPresetCount();
}