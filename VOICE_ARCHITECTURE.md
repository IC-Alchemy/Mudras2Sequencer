# Modular Voice Architecture - Programmers Manual

## Overview

This document outlines the new modular voice architecture for the Mudras2Sequencer project. The redesign transforms the monolithic voice system into a flexible, component-based architecture while maintaining full backward compatibility with the existing UI and project goals.

## Current Architecture Analysis

### Existing System
- **Monolithic Voice Class**: Fixed DSP components hardcoded in Voice.cpp
- **Static Configuration**: VoiceConfig struct with predefined parameters
- **Limited Modularity**: Fixed signal chain order
- **Parameter Management**: Centralized through ParameterManager.cpp
- **Voice Management**: VoiceManager handles voice allocation and state

### Issues Identified
1. Inflexible signal routing
2. Difficulty adding new DSP modules
3. Missing particle synthesis support
4. Duplicate struct definitions
5. Limited runtime reconfiguration

## New Modular Architecture

### Core Design Principles

1. **Component-Based Design**: Each DSP element is a separate, interchangeable component
2. **Signal Chain Flexibility**: Dynamic processor ordering and routing
3. **Backward Compatibility**: Existing presets and UI continue to work
4. **Performance Optimization**: Minimal overhead for modular design
5. **Extensibility**: Easy addition of new DSP modules

### Architecture Components

#### 1. Abstract Base Classes

```cpp
// Base class for all audio processors
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
};

// Specialized base classes
class AudioSource : public AudioProcessor {
public:
    virtual float process() = 0; // Sources don't take input
    float process(float input) override { return process(); }
};

class AudioFilter : public AudioProcessor {
public:
    virtual void setFrequency(float freq) = 0;
    virtual void setResonance(float res) = 0;
};

class AudioEffect : public AudioProcessor {
public:
    virtual void setMix(float mix) = 0;
};

class AudioEnvelope : public AudioProcessor {
public:
    virtual void trigger(bool gate) = 0;
    virtual bool isActive() const = 0;
};
```

#### 2. Signal Chain System

```cpp
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
    
    // Parameter routing
    void setProcessorParameter(size_t index, const std::string& param, float value);
    bool getProcessorParameter(size_t index, const std::string& param, float& value) const;
};
```

#### 3. Enhanced Configuration System

```cpp
struct ModularVoiceConfig {
    // Backward compatibility
    VoiceConfig legacyConfig;
    
    // New modular configuration
    struct SourceConfig {
        std::string type; // "oscillator", "particle", "noise", "vosim"
        std::map<std::string, float> parameters;
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
    
    // Signal routing matrix
    struct RoutingConfig {
        std::vector<std::pair<size_t, size_t>> connections; // (from, to) indices
        float mixLevels[8][8]; // 8x8 mixing matrix
    } routing;
    
    // Conversion functions for backward compatibility
    static ModularVoiceConfig fromLegacy(const VoiceConfig& legacy);
    VoiceConfig toLegacy() const;
};
```

#### 4. Voice Factory System

```cpp
class VoiceFactory {
public:
    static std::unique_ptr<ModularVoice> createVoice(uint8_t id, const ModularVoiceConfig& config);
    static std::unique_ptr<ModularVoice> createFromPreset(uint8_t id, const std::string& presetName);
    static std::unique_ptr<ModularVoice> createFromLegacy(uint8_t id, const VoiceConfig& legacyConfig);
    
    // Preset management
    static void registerPreset(const std::string& name, const ModularVoiceConfig& config);
    static std::vector<std::string> getAvailablePresets();
    static bool loadPreset(const std::string& name, ModularVoiceConfig& config);
};
```

#### 5. Modular Voice Implementation

```cpp
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
    
public:
    ModularVoice(uint8_t id, const ModularVoiceConfig& cfg);
    
    // Core audio processing
    void init(float sampleRate);
    float process();
    
    // Configuration management
    void setConfig(const ModularVoiceConfig& config);
    const ModularVoiceConfig& getConfig() const;
    
    // Parameter updates (maintains compatibility)
    void updateParameters(const VoiceState& newState);
    
    // Legacy compatibility methods
    void setLegacyConfig(const VoiceConfig& config);
    VoiceConfig getLegacyConfig() const;
    
    // Dynamic reconfiguration
    void addSource(const std::string& type, const std::map<std::string, float>& params);
    void addFilter(const std::string& type, const std::map<std::string, float>& params);
    void addEffect(const std::string& type, const std::map<std::string, float>& params);
    void removeProcessor(const std::string& chainType, size_t index);
    
    // Real-time parameter control
    void setSourceParameter(size_t sourceIndex, const std::string& param, float value);
    void setFilterParameter(size_t filterIndex, const std::string& param, float value);
    void setEffectParameter(size_t effectIndex, const std::string& param, float value);
};
```

## Implementation Strategy

### Phase 1: Foundation (Backward Compatibility)

1. **Create Abstract Base Classes**
   - Implement AudioProcessor hierarchy
   - Ensure minimal performance overhead
   
2. **Wrap Existing DSP Modules**
   - Create wrapper classes for current oscillators, filters, effects
   - Maintain exact same behavior as current implementation
   
3. **Implement ModularVoice with Legacy Mode**
   - ModularVoice can operate in "legacy mode" using existing Voice class
   - Gradual migration path for existing presets

### Phase 2: Core Modular System

1. **Signal Chain Implementation**
   - Dynamic processor management
   - Efficient audio processing loop
   
2. **Configuration System**
   - ModularVoiceConfig with legacy conversion
   - Preset management system
   
3. **Voice Factory**
   - Preset-based voice creation
   - Legacy compatibility layer

### Phase 3: Enhanced Features

1. **Advanced Routing**
   - Parallel processing support
   - Complex signal routing matrices
   
2. **Real-time Reconfiguration**
   - Dynamic processor addition/removal
   - Live parameter morphing
   
3. **Extended DSP Library**
   - New oscillator types
   - Advanced filter models
   - Creative effects

## Compatibility Guarantees

### UI Compatibility
- All existing parameter controls continue to work
- VoiceState structure remains unchanged
- ParameterManager integration maintained
- Preset system backward compatible

### Performance Compatibility
- No performance degradation in legacy mode
- Modular mode optimized for minimal overhead
- Memory usage controlled and predictable

### API Compatibility
- VoiceManager interface unchanged
- Existing voice creation methods supported
- Parameter update mechanisms preserved

## Integration with Existing Systems

### ParameterManager Integration

```cpp
// Enhanced parameter mapping for modular voices
class ModularParameterMapper {
public:
    // Maps ParameterManager values to modular voice parameters
    static void mapParameters(const VoiceState& state, ModularVoice& voice);
    
    // Backward compatibility mapping
    static void mapLegacyParameters(const VoiceState& state, Voice& voice);
    
    // Real-time parameter updates
    static void updateParameter(ParamId id, float value, ModularVoice& voice);
};
```

### VoiceManager Integration

```cpp
class EnhancedVoiceManager : public VoiceManager {
private:
    std::vector<std::unique_ptr<ModularVoice>> modularVoices;
    bool useModularVoices = false;
    
public:
    // Backward compatible methods
    void updateVoiceState(uint8_t voiceId, const VoiceState& state) override;
    void setVoiceFrequency(uint8_t voiceId, float frequency) override;
    
    // New modular methods
    void enableModularMode(bool enable);
    void setVoiceConfig(uint8_t voiceId, const ModularVoiceConfig& config);
    void loadVoicePreset(uint8_t voiceId, const std::string& presetName);
};
```

## Migration Path

### Immediate (Phase 1)
1. Existing code continues to work unchanged
2. New modular system available as opt-in
3. Performance testing and validation

### Short-term (Phase 2)
1. Gradual migration of presets to modular format
2. Enhanced UI features for modular voices
3. User testing and feedback integration

### Long-term (Phase 3)
1. Full modular system as default
2. Legacy mode for backward compatibility
3. Advanced features and optimizations

## Benefits

### For Developers
- **Easier Maintenance**: Modular code is easier to debug and extend
- **Faster Development**: New DSP modules can be added quickly
- **Better Testing**: Individual components can be tested in isolation
- **Code Reuse**: Components can be shared between different voice types

### For Users
- **More Sound Options**: Easy access to different synthesis methods
- **Customization**: Ability to create custom voice configurations
- **Performance**: Optimized signal chains for better CPU usage
- **Compatibility**: Existing patches and presets continue to work

### For the Project
- **Scalability**: Architecture supports future expansion
- **Maintainability**: Cleaner, more organized codebase
- **Innovation**: Platform for experimenting with new synthesis techniques
- **Community**: Easier for contributors to add new features

## Implementation Status

**Current Phase: Core Implementation Complete**

The modular voice architecture has been implemented with the following components:

### Completed Components

#### Core Architecture Files
- **`ModularVoice.h`**: Complete modular voice architecture with abstract base classes, signal chain system, and enhanced configuration
- **`ModularVoice.cpp`**: Full implementation of modular voice system with legacy compatibility
- **`AudioProcessors.h`**: Concrete implementations of all audio processors
- **`AudioProcessors.cpp`**: Complete DSP implementations for all modular components

#### Implemented Features
- ✅ Abstract base classes (`AudioProcessor`, `AudioSource`, `AudioFilter`, `AudioEffect`, `AudioEnvelope`)
- ✅ Signal chain system with dynamic routing and bypass capabilities
- ✅ Enhanced `ModularVoiceConfig` with backward compatibility
- ✅ Factory pattern for creating audio processors and voices
- ✅ Parameter mapping system for integration with existing `ParameterManager`
- ✅ Complete set of modular audio processors (Oscillator, Particle, Filters, Effects, Envelopes)
- ✅ Legacy compatibility layer for seamless transition
- ✅ Enhanced voice manager for modular voice orchestration

### Integration Status

#### Parameter System Integration
The new architecture fully integrates with the existing parameter system:
- **`ParamId` Mapping**: All existing parameter IDs (Note, Velocity, Filter, Attack, Decay, etc.) are mapped to modular voice parameters
- **`ParameterManager` Compatibility**: The `ModularParameterMapper` class ensures seamless integration with `ParameterManager.cpp`
- **Thread-Safe Access**: Maintains the same thread-safe parameter access patterns as the original system

#### Voice State Compatibility
The `VoiceState` structure remains unchanged, ensuring:
- **UI Compatibility**: All existing UI controls continue to work without modification
- **Sequencer Integration**: Full compatibility with the existing sequencer system
- **Parameter Ranges**: All parameter ranges and behaviors are preserved

### Next Steps

1. **Integration Testing**: Test the new modular system with existing UI and sequencer
2. **Performance Optimization**: Profile and optimize the modular voice processing
3. **Preset Migration**: Create modular versions of existing voice presets
4. **Advanced Features**: Implement dynamic voice reconfiguration and custom routing
5. **Documentation**: Create user and developer guides for the new system

## Conclusion

The new modular voice architecture provides a solid foundation for the future development of the Mudras2Sequencer while maintaining full backward compatibility. The implementation is now complete and ready for integration testing.

The architecture is designed to be:
- **Flexible**: Easy to reconfigure and extend
- **Performant**: Optimized for real-time audio processing
- **Compatible**: Works with existing UI and parameter systems
- **Future-proof**: Supports advanced features and new DSP modules

This implementation positions the Mudras2Sequencer as a powerful, extensible synthesizer platform while preserving the investment in existing code and user workflows.