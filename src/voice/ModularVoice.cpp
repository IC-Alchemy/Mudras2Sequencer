#include "ModularVoice.h"
#include "AudioProcessors.h"
#include "../scales/scales.h"
#include "../dsp/dsp.h"
#include "Arduino.h"
#include <algorithm>
#include <cmath>

// Signal Chain Implementation
void SignalChain::init(float sr) {
    sampleRate = sr;
    for (auto& processor : processors) {
        if (processor) {
            processor->init(sampleRate);
        }
    }
}

void SignalChain::addProcessor(std::unique_ptr<AudioProcessor> processor) {
    if (processor) {
        processor->init(sampleRate);
        processors.push_back(std::move(processor));
        bypassStates.push_back(false);
    }
}

void SignalChain::removeProcessor(size_t index) {
    if (index < processors.size()) {
        processors.erase(processors.begin() + index);
        bypassStates.erase(bypassStates.begin() + index);
    }
}

void SignalChain::moveProcessor(size_t from, size_t to) {
    if (from < processors.size() && to < processors.size() && from != to) {
        std::swap(processors[from], processors[to]);
        bool temp = bypassStates[from];
        bypassStates[from] = bypassStates[to];
        bypassStates[to] = temp;
    }
}

void SignalChain::setBypass(size_t index, bool bypass) {
    if (index < bypassStates.size()) {
        bypassStates[index] = bypass;
    }
}

float SignalChain::process(float input) {
    float signal = input;
    for (size_t i = 0; i < processors.size(); ++i) {
        if (processors[i] && processors[i]->isEnabled() && !bypassStates[i]) {
            signal = processors[i]->process(signal);
        }
    }
    return signal;
}

void SignalChain::reset() {
    for (auto& processor : processors) {
        if (processor) {
            processor->reset();
        }
    }
}

void SignalChain::setProcessorParameter(size_t index, const std::string& param, float value) {
    if (index < processors.size() && processors[index]) {
        processors[index]->setParameter(param, value);
    }
}

bool SignalChain::getProcessorParameter(size_t index, const std::string& param, float& value) const {
    if (index < processors.size() && processors[index]) {
        return processors[index]->getParameter(param, value);
    }
    return false;
}

AudioProcessor* SignalChain::getProcessor(size_t index) const {
    if (index < processors.size()) {
        return processors[index].get();
    }
    return nullptr;
}

// ModularVoiceConfig Implementation
ModularVoiceConfig ModularVoiceConfig::fromLegacy(const VoiceConfig& legacy) {
    ModularVoiceConfig config;
    config.legacyConfig = legacy;
    config.useModularMode = false; // Start in legacy mode
    config.outputLevel = legacy.outputLevel;
    config.enabled = legacy.enabled;
    
    // Convert oscillators to sources
    for (uint8_t i = 0; i < legacy.oscillatorCount && i < 3; ++i) {
        SourceConfig oscConfig;
        oscConfig.type = "oscillator";
        oscConfig.parameters["waveform"] = static_cast<float>(legacy.oscWaveforms[i]);
        oscConfig.parameters["amplitude"] = legacy.oscAmplitudes[i];
        oscConfig.parameters["detuning"] = legacy.oscDetuning[i];
        oscConfig.parameters["pulsewidth"] = legacy.oscPulseWidth[i];
        oscConfig.enabled = true;
        config.sources.push_back(oscConfig);
    }
    
    // Convert filter settings
    ProcessorConfig filterConfig;
    filterConfig.type = "ladder_filter";
    filterConfig.parameters["resonance"] = legacy.filterRes;
    filterConfig.parameters["drive"] = legacy.filterDrive;
    filterConfig.parameters["passband_gain"] = legacy.filterPassbandGain;
    filterConfig.enabled = true;
    config.filters.push_back(filterConfig);
    
    // Add high-pass filter
    ProcessorConfig hpfConfig;
    hpfConfig.type = "svf_filter";
    hpfConfig.parameters["frequency"] = legacy.highPassFreq;
    hpfConfig.parameters["mode"] = 1.0f; // High-pass mode
    hpfConfig.enabled = true;
    config.filters.push_back(hpfConfig);
    
    // Convert effects
    if (legacy.hasOverdrive) {
        ProcessorConfig overdriveConfig;
        overdriveConfig.type = "overdrive";
        overdriveConfig.parameters["drive"] = legacy.overdriveAmount;
        overdriveConfig.parameters["mix"] = 1.0f;
        overdriveConfig.enabled = true;
        config.effects.push_back(overdriveConfig);
    }
    
    if (legacy.hasWavefolder) {
        ProcessorConfig wavefolderConfig;
        wavefolderConfig.type = "wavefolder";
        wavefolderConfig.parameters["gain"] = legacy.wavefolderGain;
        wavefolderConfig.parameters["offset"] = legacy.wavefolderOffset;
        wavefolderConfig.parameters["mix"] = 1.0f;
        wavefolderConfig.enabled = true;
        config.effects.push_back(wavefolderConfig);
    }
    
    // Convert envelope
    ProcessorConfig envelopeConfig;
    envelopeConfig.type = "adsr";
    envelopeConfig.parameters["attack"] = legacy.defaultAttack;
    envelopeConfig.parameters["decay"] = legacy.defaultDecay;
    envelopeConfig.parameters["sustain"] = legacy.defaultSustain;
    envelopeConfig.parameters["release"] = legacy.defaultRelease;
    envelopeConfig.enabled = true;
    config.envelopes.push_back(envelopeConfig);
    
    return config;
}

VoiceConfig ModularVoiceConfig::toLegacy() const {
    VoiceConfig legacy = legacyConfig;
    
    // Update basic settings
    legacy.outputLevel = outputLevel;
    legacy.enabled = enabled;
    
    // Convert sources back to oscillators
    legacy.oscillatorCount = 0;
    for (size_t i = 0; i < sources.size() && i < 3; ++i) {
        if (sources[i].type == "oscillator" && sources[i].enabled) {
            auto it = sources[i].parameters.find("waveform");
            if (it != sources[i].parameters.end()) {
                legacy.oscWaveforms[legacy.oscillatorCount] = static_cast<uint8_t>(it->second);
            }
            
            it = sources[i].parameters.find("amplitude");
            if (it != sources[i].parameters.end()) {
                legacy.oscAmplitudes[legacy.oscillatorCount] = it->second;
            }
            
            it = sources[i].parameters.find("detuning");
            if (it != sources[i].parameters.end()) {
                legacy.oscDetuning[legacy.oscillatorCount] = it->second;
            }
            
            it = sources[i].parameters.find("pulsewidth");
            if (it != sources[i].parameters.end()) {
                legacy.oscPulseWidth[legacy.oscillatorCount] = it->second;
            }
            
            legacy.oscillatorCount++;
        }
    }
    
    // Convert effects back
    legacy.hasOverdrive = false;
    legacy.hasWavefolder = false;
    
    for (const auto& effect : effects) {
        if (effect.type == "overdrive" && effect.enabled) {
            legacy.hasOverdrive = true;
            auto it = effect.parameters.find("drive");
            if (it != effect.parameters.end()) {
                legacy.overdriveAmount = it->second;
            }
        } else if (effect.type == "wavefolder" && effect.enabled) {
            legacy.hasWavefolder = true;
            auto it = effect.parameters.find("gain");
            if (it != effect.parameters.end()) {
                legacy.wavefolderGain = it->second;
            }
            it = effect.parameters.find("offset");
            if (it != effect.parameters.end()) {
                legacy.wavefolderOffset = it->second;
            }
        }
    }
    
    return legacy;
}

// AudioProcessorFactory Implementation
std::unique_ptr<AudioSource> AudioProcessorFactory::createOscillator(const std::map<std::string, float>& params) {
    auto osc = std::make_unique<ModularOscillator>();
    for (const auto& param : params) {
        osc->setParameter(param.first, param.second);
    }
    return osc;
}

std::unique_ptr<AudioSource> AudioProcessorFactory::createParticle(const std::map<std::string, float>& params) {
    auto particle = std::make_unique<ModularParticle>();
    for (const auto& param : params) {
        particle->setParameter(param.first, param.second);
    }
    return particle;
}

std::unique_ptr<AudioSource> AudioProcessorFactory::createNoise(const std::map<std::string, float>& params) {
    auto noise = std::make_unique<ModularNoise>();
    for (const auto& param : params) {
        noise->setParameter(param.first, param.second);
    }
    return noise;
}

std::unique_ptr<AudioSource> AudioProcessorFactory::createVosim(const std::map<std::string, float>& params) {
    auto vosim = std::make_unique<ModularVosim>();
    for (const auto& param : params) {
        vosim->setParameter(param.first, param.second);
    }
    return vosim;
}

std::unique_ptr<AudioFilter> AudioProcessorFactory::createLadderFilter(const std::map<std::string, float>& params) {
    auto filter = std::make_unique<ModularLadderFilter>();
    for (const auto& param : params) {
        filter->setParameter(param.first, param.second);
    }
    return filter;
}

std::unique_ptr<AudioFilter> AudioProcessorFactory::createSvfFilter(const std::map<std::string, float>& params) {
    auto filter = std::make_unique<ModularSvfFilter>();
    for (const auto& param : params) {
        filter->setParameter(param.first, param.second);
    }
    return filter;
}

std::unique_ptr<AudioEffect> AudioProcessorFactory::createOverdrive(const std::map<std::string, float>& params) {
    auto overdrive = std::make_unique<ModularOverdrive>();
    for (const auto& param : params) {
        overdrive->setParameter(param.first, param.second);
    }
    return overdrive;
}

std::unique_ptr<AudioEffect> AudioProcessorFactory::createWavefolder(const std::map<std::string, float>& params) {
    auto wavefolder = std::make_unique<ModularWavefolder>();
    for (const auto& param : params) {
        wavefolder->setParameter(param.first, param.second);
    }
    return wavefolder;
}

std::unique_ptr<AudioEffect> AudioProcessorFactory::createTremolo(const std::map<std::string, float>& params) {
    auto tremolo = std::make_unique<ModularTremolo>();
    for (const auto& param : params) {
        tremolo->setParameter(param.first, param.second);
    }
    return tremolo;
}

std::unique_ptr<AudioEnvelope> AudioProcessorFactory::createAdsr(const std::map<std::string, float>& params) {
    auto adsr = std::make_unique<ModularAdsr>();
    for (const auto& param : params) {
        adsr->setParameter(param.first, param.second);
    }
    return adsr;
}

std::unique_ptr<AudioProcessor> AudioProcessorFactory::createProcessor(const std::string& type, const std::map<std::string, float>& params) {
    if (type == "oscillator") {
        return createOscillator(params);
    } else if (type == "particle") {
        return createParticle(params);
    } else if (type == "noise") {
        return createNoise(params);
    } else if (type == "vosim") {
        return createVosim(params);
    } else if (type == "ladder_filter") {
        return createLadderFilter(params);
    } else if (type == "svf_filter") {
        return createSvfFilter(params);
    } else if (type == "overdrive") {
        return createOverdrive(params);
    } else if (type == "wavefolder") {
        return createWavefolder(params);
    } else if (type == "tremolo") {
        return createTremolo(params);
    } else if (type == "adsr") {
        return createAdsr(params);
    }
    return nullptr;
}

// ModularVoice Implementation
ModularVoice::ModularVoice(uint8_t id, const ModularVoiceConfig& cfg) 
    : voiceId(id), config(cfg), sampleRate(48000.0f), finalOutput(0.0f), gate(false) {
    
    // Initialize source mix levels
    for (int i = 0; i < 8; i++) {
        sourceMix[i] = config.routing.sourceMixLevels[i];
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
    state.gateLength = 24;
    
    // Create legacy voice for fallback
    if (!config.useModularMode) {
        legacyVoice = std::make_unique<Voice>(id, config.legacyConfig);
    }
}

void ModularVoice::init(float sr) {
    sampleRate = sr;
    
    if (!config.useModularMode && legacyVoice) {
        legacyVoice->init(sampleRate);
        return;
    }
    
    // Initialize modular components
    
    // Create sources
    sources.clear();
    for (const auto& sourceConfig : config.sources) {
        if (sourceConfig.enabled) {
            auto source = AudioProcessorFactory::createProcessor(sourceConfig.type, sourceConfig.parameters);
            if (source) {
                auto audioSource = std::unique_ptr<AudioSource>(static_cast<AudioSource*>(source.release()));
                if (audioSource) {
                    audioSource->init(sampleRate);
                    sources.push_back(std::move(audioSource));
                }
            }
        }
    }
    
    // Initialize filter chain
    filterChain.init(sampleRate);
    for (const auto& filterConfig : config.filters) {
        if (filterConfig.enabled) {
            auto filter = AudioProcessorFactory::createProcessor(filterConfig.type, filterConfig.parameters);
            if (filter) {
                filterChain.addProcessor(std::move(filter));
            }
        }
    }
    
    // Initialize effects chain
    effectsChain.init(sampleRate);
    for (const auto& effectConfig : config.effects) {
        if (effectConfig.enabled) {
            auto effect = AudioProcessorFactory::createProcessor(effectConfig.type, effectConfig.parameters);
            if (effect) {
                effectsChain.addProcessor(std::move(effect));
            }
        }
    }
    
    // Create envelopes
    envelopes.clear();
    for (const auto& envelopeConfig : config.envelopes) {
        if (envelopeConfig.enabled) {
            auto envelope = AudioProcessorFactory::createProcessor(envelopeConfig.type, envelopeConfig.parameters);
            if (envelope) {
                auto audioEnvelope = std::unique_ptr<AudioEnvelope>(static_cast<AudioEnvelope*>(envelope.release()));
                if (audioEnvelope) {
                    audioEnvelope->init(sampleRate);
                    envelopes.push_back(std::move(audioEnvelope));
                }
            }
        }
    }
}

float ModularVoice::process() {
    if (!config.enabled) {
        return 0.0f;
    }
    
    if (!config.useModularMode && legacyVoice) {
        return legacyVoice->process();
    }
    
    return processModularMode();
}

float ModularVoice::processModularMode() {
    // Handle envelope retrigger
    if (state.retrigger) {
        for (auto& envelope : envelopes) {
            envelope->retrigger();
        }
        state.retrigger = false;
    }
    
    // Process envelopes
    for (auto& envelope : envelopes) {
        envelope->trigger(gate);
    }
    
    // Mix sources
    float mixedSources = 0.0f;
    for (size_t i = 0; i < sources.size() && i < 8; ++i) {
        if (sources[i] && sources[i]->isEnabled()) {
            float sourceOutput = sources[i]->process();
            mixedSources += sourceOutput * sourceMix[i];
        }
    }
    
    // Process through filter chain
    float filteredSignal = filterChain.process(mixedSources);
    
    // Process through effects chain
    float effectsSignal = effectsChain.process(filteredSignal);
    
    // Apply envelopes
    float envelopedSignal = effectsSignal;
    for (auto& envelope : envelopes) {
        if (envelope && envelope->isEnabled()) {
            envelopedSignal = envelope->process(envelopedSignal);
        }
    }
    
    finalOutput = envelopedSignal * config.outputLevel;
    return finalOutput;
}

void ModularVoice::processLegacyMode() {
    if (legacyVoice) {
        finalOutput = legacyVoice->process();
    } else {
        finalOutput = 0.0f;
    }
}

void ModularVoice::updateParameters(const VoiceState& newState) {
    state = newState;
    gate = state.gate;
    
    if (!config.useModularMode && legacyVoice) {
        legacyVoice->updateParameters(newState);
        return;
    }
    
    updateSourceParameters();
    updateFilterParameters();
    updateEffectParameters();
    updateEnvelopeParameters();
}

void ModularVoice::updateSourceParameters() {
    // Calculate base frequency from note and octave
    extern int scale[7][48];
    extern uint8_t currentScale;
    
    int noteIndex = std::max(0, std::min(static_cast<int>(state.note), 47));
    int scaleNote = scale[currentScale][noteIndex];
    float midiNote = scaleNote + 48 + state.octave;
    float baseFreq = daisysp::mtof(midiNote);
    
    // Update source frequencies and amplitudes
    for (size_t i = 0; i < sources.size(); ++i) {
        if (sources[i] && sources[i]->isEnabled()) {
            // Apply detuning if specified in config
            float detuning = 0.0f;
            if (i < config.sources.size()) {
                auto it = config.sources[i].parameters.find("detuning");
                if (it != config.sources[i].parameters.end()) {
                    detuning = it->second;
                }
            }
            
            float detuneMultiplier = std::pow(2.0f, detuning / 12.0f);
            float targetFreq = baseFreq * detuneMultiplier;
            
            sources[i]->setFrequency(targetFreq);
            sources[i]->setAmplitude(state.velocity);
            
            // Update particle-specific parameters
            if (sources[i]->getType() == "particle") {
                // Filter Parameter → Particle Resonance (0.1 to 0.95)
                float resonance = daisysp::fmap(state.filter, 0.1f, 0.95f, daisysp::Mapping::LINEAR);
                sources[i]->setParameter("resonance", resonance);
                
                // Attack Parameter → Particle Density (inverted)
                float density = daisysp::fmap(1.0f - state.attack, 0.2f, 1.0f, daisysp::Mapping::LINEAR);
                sources[i]->setParameter("density", density);
            }
        }
    }
}

void ModularVoice::updateFilterParameters() {
    // Standard filter frequency calculation
    float filterFrequency = daisysp::fmap(state.filter, 150.0f, 11710.0f, daisysp::Mapping::EXP);
    
    // Update all filters in the chain
    for (size_t i = 0; i < filterChain.getProcessorCount(); ++i) {
        AudioProcessor* processor = filterChain.getProcessor(i);
        if (processor && processor->getType().find("filter") != std::string::npos) {
            processor->setParameter("frequency", filterFrequency);
        }
    }
}

void ModularVoice::updateEffectParameters() {
    // Update effect parameters based on voice state
    // This can be extended to map voice parameters to effect parameters
}

void ModularVoice::updateEnvelopeParameters() {
    // Map normalized parameters to appropriate ranges
    float attack = daisysp::fmap(state.attack, 0.005f, 0.75f, daisysp::Mapping::LINEAR);
    float decay = daisysp::fmap(state.decay, 0.01f, 0.6f, daisysp::Mapping::LINEAR);
    float release = decay; // Use decay for release
    
    for (auto& envelope : envelopes) {
        if (envelope && envelope->isEnabled()) {
            envelope->setParameter("attack", attack);
            envelope->setParameter("decay", 0.05f + (release * 0.5f));
            envelope->setParameter("release", release);
        }
    }
}

void ModularVoice::setConfig(const ModularVoiceConfig& cfg) {
    config = cfg;
    
    // Update legacy voice if in legacy mode
    if (!config.useModularMode) {
        if (!legacyVoice) {
            legacyVoice = std::make_unique<Voice>(voiceId, config.legacyConfig);
            legacyVoice->init(sampleRate);
        } else {
            legacyVoice->setConfig(config.legacyConfig);
        }
    } else {
        // Reinitialize modular components
        init(sampleRate);
    }
}

void ModularVoice::setLegacyConfig(const VoiceConfig& legacyConfig) {
    config.legacyConfig = legacyConfig;
    config.useModularMode = false;
    
    if (!legacyVoice) {
        legacyVoice = std::make_unique<Voice>(voiceId, legacyConfig);
        legacyVoice->init(sampleRate);
    } else {
        legacyVoice->setConfig(legacyConfig);
    }
}

VoiceConfig ModularVoice::getLegacyConfig() const {
    if (legacyVoice) {
        return legacyVoice->getConfig();
    }
    return config.toLegacy();
}

void ModularVoice::setFrequency(float frequency) {
    if (!config.useModularMode && legacyVoice) {
        legacyVoice->setFrequency(frequency);
        return;
    }
    
    for (auto& source : sources) {
        if (source && source->isEnabled()) {
            source->setFrequency(frequency);
        }
    }
}

void ModularVoice::setSlideTime(float slideTime) {
    if (!config.useModularMode && legacyVoice) {
        legacyVoice->setSlideTime(slideTime);
    }
    // TODO: Implement slide time for modular mode
}

void ModularVoice::setSequencer(std::unique_ptr<Sequencer> seq) {
    if (legacyVoice) {
        legacyVoice->setSequencer(std::move(seq));
    }
}

void ModularVoice::setSequencer(Sequencer* seq) {
    if (legacyVoice) {
        legacyVoice->setSequencer(seq);
    }
}

// Dynamic reconfiguration methods
void ModularVoice::addSource(const std::string& type, const std::map<std::string, float>& params) {
    if (!config.useModularMode) return;
    
    auto source = AudioProcessorFactory::createProcessor(type, params);
    if (source) {
        auto audioSource = std::unique_ptr<AudioSource>(static_cast<AudioSource*>(source.release()));
        if (audioSource) {
            audioSource->init(sampleRate);
            sources.push_back(std::move(audioSource));
            
            // Add to config
            ModularVoiceConfig::SourceConfig sourceConfig;
            sourceConfig.type = type;
            sourceConfig.parameters = params;
            sourceConfig.enabled = true;
            config.sources.push_back(sourceConfig);
        }
    }
}

void ModularVoice::addFilter(const std::string& type, const std::map<std::string, float>& params) {
    if (!config.useModularMode) return;
    
    auto filter = AudioProcessorFactory::createProcessor(type, params);
    if (filter) {
        filterChain.addProcessor(std::move(filter));
        
        // Add to config
        ModularVoiceConfig::ProcessorConfig filterConfig;
        filterConfig.type = type;
        filterConfig.parameters = params;
        filterConfig.enabled = true;
        config.filters.push_back(filterConfig);
    }
}

void ModularVoice::addEffect(const std::string& type, const std::map<std::string, float>& params) {
    if (!config.useModularMode) return;
    
    auto effect = AudioProcessorFactory::createProcessor(type, params);
    if (effect) {
        effectsChain.addProcessor(std::move(effect));
        
        // Add to config
        ModularVoiceConfig::ProcessorConfig effectConfig;
        effectConfig.type = type;
        effectConfig.parameters = params;
        effectConfig.enabled = true;
        config.effects.push_back(effectConfig);
    }
}

void ModularVoice::removeProcessor(const std::string& chainType, size_t index) {
    if (!config.useModularMode) return;
    
    if (chainType == "filter") {
        filterChain.removeProcessor(index);
        if (index < config.filters.size()) {
            config.filters.erase(config.filters.begin() + index);
        }
    } else if (chainType == "effect") {
        effectsChain.removeProcessor(index);
        if (index < config.effects.size()) {
            config.effects.erase(config.effects.begin() + index);
        }
    } else if (chainType == "source") {
        if (index < sources.size()) {
            sources.erase(sources.begin() + index);
        }
        if (index < config.sources.size()) {
            config.sources.erase(config.sources.begin() + index);
        }
    }
}

void ModularVoice::setSourceParameter(size_t sourceIndex, const std::string& param, float value) {
    if (sourceIndex < sources.size() && sources[sourceIndex]) {
        sources[sourceIndex]->setParameter(param, value);
    }
}

void ModularVoice::setFilterParameter(size_t filterIndex, const std::string& param, float value) {
    filterChain.setProcessorParameter(filterIndex, param, value);
}

void ModularVoice::setEffectParameter(size_t effectIndex, const std::string& param, float value) {
    effectsChain.setProcessorParameter(effectIndex, param, value);
}

// ModularParameterMapper Implementation
void ModularParameterMapper::mapParameters(const VoiceState& state, ModularVoice& voice) {
    voice.updateParameters(state);
}

void ModularParameterMapper::mapLegacyParameters(const VoiceState& state, Voice& voice) {
    voice.updateParameters(state);
}

void ModularParameterMapper::updateParameter(ParamId id, float value, ModularVoice& voice) {
    VoiceState& state = voice.getState();
    
    switch (id) {
        case ParamId::Note:
            state.note = value;
            break;
        case ParamId::Velocity:
            state.velocity = value;
            break;
        case ParamId::Filter:
            state.filter = value;
            break;
        case ParamId::Attack:
            state.attack = value;
            break;
        case ParamId::Decay:
            state.decay = value;
            break;
        case ParamId::Octave:
            state.octave = value;
            break;
        case ParamId::Gate:
            state.gate = value > 0.5f;
            break;
        case ParamId::Slide:
            state.slide = value > 0.5f;
            break;
        default:
            break;
    }
    
    voice.updateParameters(state);
}

// VoiceFactory Implementation
namespace VoiceFactory {
    std::unique_ptr<ModularVoice> createVoice(uint8_t id, const ModularVoiceConfig& config) {
        return std::make_unique<ModularVoice>(id, config);
    }

    std::unique_ptr<ModularVoice> createFromPreset(uint8_t id, const std::string& presetName) {
        ModularVoiceConfig config;
        if (loadPreset(presetName, config)) {
            return createVoice(id, config);
        }
        
        // Fallback to analog voice
        VoiceConfig legacyConfig = VoicePresets::getAnalogVoice();
        config = ModularVoiceConfig::fromLegacy(legacyConfig);
        return createVoice(id, config);
    }

    std::unique_ptr<ModularVoice> createFromLegacy(uint8_t id, const VoiceConfig& legacyConfig) {
        ModularVoiceConfig config = ModularVoiceConfig::fromLegacy(legacyConfig);
        return createVoice(id, config);
    }

    // Static preset storage
    static std::map<std::string, ModularVoiceConfig> presetRegistry;

    void registerPreset(const std::string& name, const ModularVoiceConfig& config) {
        presetRegistry[name] = config;
    }

    std::vector<std::string> getAvailablePresets() {
        std::vector<std::string> presets;
        
        // Add legacy presets
        for (uint8_t i = 0; i < VoicePresets::getPresetCount(); ++i) {
            presets.push_back(VoicePresets::getPresetName(i));
        }
        
        // Add modular presets
        for (const auto& preset : presetRegistry) {
            presets.push_back(preset.first);
        }
        
        return presets;
    }

    bool loadPreset(const std::string& name, ModularVoiceConfig& config) {
        // Check modular presets first
        auto it = presetRegistry.find(name);
        if (it != presetRegistry.end()) {
            config = it->second;
            return true;
        }
        
        // Check legacy presets
        for (uint8_t i = 0; i < VoicePresets::getPresetCount(); ++i) {
            if (name == VoicePresets::getPresetName(i)) {
                VoiceConfig legacyConfig = VoicePresets::getPresetConfig(i);
                config = ModularVoiceConfig::fromLegacy(legacyConfig);
                return true;
            }
        }
        
        return false;
    }
}

// EnhancedVoiceManager Implementation
namespace EnhancedVoiceManager {
    static std::vector<std::unique_ptr<ModularVoice>> modularVoices;
    
    void init(float sampleRate) {
        for (auto& voice : modularVoices) {
            if (voice) {
                voice->init(sampleRate);
            }
        }
    }

    void createVoice(uint8_t voiceId, const ModularVoiceConfig& config) {
        if (voiceId >= modularVoices.size()) {
            modularVoices.resize(voiceId + 1);
        }
        
        modularVoices[voiceId] = VoiceFactory::createVoice(voiceId, config);
        if (modularVoices[voiceId]) {
            modularVoices[voiceId]->init(48000.0f); // Default sample rate
        }
    }

    void createVoiceFromLegacy(uint8_t voiceId, const VoiceConfig& legacyConfig) {
        ModularVoiceConfig config = ModularVoiceConfig::fromLegacy(legacyConfig);
        createVoice(voiceId, config);
    }

    void updateVoiceState(uint8_t voiceId, const VoiceState& state) {
        if (voiceId < modularVoices.size() && modularVoices[voiceId]) {
            modularVoices[voiceId]->updateParameters(state);
        }
    }

    void setVoiceFrequency(uint8_t voiceId, float frequency) {
        if (voiceId < modularVoices.size() && modularVoices[voiceId]) {
            modularVoices[voiceId]->setFrequency(frequency);
        }
    }

    float processVoice(uint8_t voiceId) {
        if (voiceId < modularVoices.size() && modularVoices[voiceId]) {
            return modularVoices[voiceId]->process();
        }
        return 0.0f;
    }

    void setVoiceConfig(uint8_t voiceId, const ModularVoiceConfig& config) {
        if (voiceId < modularVoices.size() && modularVoices[voiceId]) {
            modularVoices[voiceId]->setConfig(config);
        }
    }

    void loadVoicePreset(uint8_t voiceId, const std::string& presetName) {
        ModularVoiceConfig config;
        if (VoiceFactory::loadPreset(presetName, config)) {
            if (voiceId >= modularVoices.size()) {
                modularVoices.resize(voiceId + 1);
            }
            
            if (!modularVoices[voiceId]) {
                createVoice(voiceId, config);
            } else {
                modularVoices[voiceId]->setConfig(config);
            }
        }
    }

    ModularVoice* getVoice(uint8_t voiceId) {
        if (voiceId < modularVoices.size()) {
            return modularVoices[voiceId].get();
        }
        return nullptr;
    }

    const ModularVoice* getVoice(uint8_t voiceId) const {
        if (voiceId < modularVoices.size()) {
            return modularVoices[voiceId].get();
        }
        return nullptr;
    }

    float processAllVoices() {
        float mixed = 0.0f;
        for (auto& v : modularVoices) {
            if (v) {
                mixed += v->process();
            }
        }
        return mixed;
    }

    void setVoiceSlide(uint8_t voiceId, float slideTime) {
        if (auto v = getVoice(voiceId)) {
            v->setSlideTime(slideTime);
        }
    }

    void setVoiceVolume(uint8_t voiceId, float volume) {
        if (auto v = getVoice(voiceId)) {
            auto cfg = v->getConfig();
            cfg.outputLevel = volume;
            v->setConfig(cfg);
        }
    }
}