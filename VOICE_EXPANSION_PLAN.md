# PicoMudrasSequencer Voice Expansion Plan
## Adding Voice 3 and Voice 4 Support

### Overview
This document outlines the comprehensive plan to extend the PicoMudrasSequencer from supporting 2 voices to 4 voices (adding Voice 3 and Voice 4).

### Current State Analysis
- **Existing Voices**: Voice 1 and Voice 2
- **Architecture**: Modular voice system with VoiceManager, individual Voice objects, and parameter management
- **Memory Usage**: ~60-70% of available RAM with current 2-voice setup
- **Processing**: Real-time audio synthesis with 48kHz sample rate

### Phase 1: Configuration Updates

#### 1.1 SequencerDefs.h Modifications
```cpp
// Update voice count constants
constexpr uint8_t MAX_VOICES = 4;        // Changed from 2
constexpr uint8_t DEFAULT_ACTIVE_VOICES = 4;  // Changed from 2

// Memory allocation constants
constexpr size_t VOICE_BUFFER_SIZE = 256;  // Per voice
constexpr size_t TOTAL_VOICE_BUFFER_SIZE = MAX_VOICES * VOICE_BUFFER_SIZE;
```

#### 1.2 Parameter Manager Updates
- Extend parameter arrays to support 4 voices
- Add parameter mapping for Voice 3 and Voice 4
- Update MIDI CC mapping to include new voice parameters

### Phase 2: Voice System Expansion

#### 2.1 VoiceManager Class Updates
```cpp
// Add new voice instances
Voice voice3;
Voice voice4;

// Update voice array management
Voice* voices[MAX_VOICES] = {&voice1, &voice2, &voice3, &voice4};
```

#### 2.2 Memory Optimization Strategy
- **Buffer Sharing**: Implement shared delay lines between voices
- **Parameter Compression**: Use bitfields for parameter storage
- **Dynamic Allocation**: Consider dynamic voice allocation based on active voices

### Phase 3: Audio Processing Integration

#### 3.1 Audio Engine Updates
- **Mixer Expansion**: Extend audio mixer to handle 4 voices
- **Gain Staging**: Adjust master gain to prevent clipping with 4 voices
- **Voice Allocation**: Implement voice stealing algorithm for polyphony

#### 3.2 Processing Optimizations
- **SIMD Operations**: Utilize ARM NEON instructions for parallel processing
- **Buffer Management**: Optimize buffer sizes for 4-voice operation
- **Interrupt Timing**: Adjust audio interrupt timing for increased load

### Phase 4: UI and Control Integration

#### 4.1 Parameter Display Updates
- **OLED Screen**: Add pages for Voice 3 and Voice 4 parameters
- **LED Matrix**: Extend visual feedback for 4 voices
- **Navigation**: Implement efficient parameter navigation across 4 voices

#### 4.2 MIDI Control Updates
- **CC Mapping**: Add new CC numbers for Voice 3 and Voice 4
- **NRPN Support**: Implement NRPN for extended parameter control
- **Polyphonic Aftertouch**: Add support for per-voice aftertouch

### Phase 5: Testing and Validation

#### 5.1 Memory Testing
```cpp
// Memory usage monitoring
void logMemoryUsage() {
    Serial.print("Free RAM: ");
    Serial.println(rp2040.getFreeHeap());
    Serial.print("Max active voices: ");
    Serial.println(MAX_VOICES);
}
```

#### 5.2 Performance Testing
- **CPU Load**: Monitor CPU usage with all 4 voices active
- **Audio Quality**: Verify no audio dropouts or artifacts
- **Latency**: Ensure consistent low-latency performance

### Phase 6: Advanced Features

#### 6.1 Voice Modes
- **Polyphonic Mode**: True 4-voice polyphony
- **Layer Mode**: Voice 1+3 and 2+4 layered pairs
- **Split Mode**: Keyboard split with 2 voices per zone

#### 6.2 Effects Integration
- **Shared Effects**: Chorus, reverb, and delay shared across voices
- **Voice-Specific Effects**: Individual filters and envelopes per voice
- **Effect Routing**: Flexible effect send/return configuration

### Implementation Timeline

| Phase | Duration | Priority | Status |
|-------|----------|----------|---------|
| Phase 1: Configuration | 1-2 days | High | Planned |
| Phase 2: Voice System | 2-3 days | High | Planned |
| Phase 3: Audio Processing | 3-4 days | High | Planned |
| Phase 4: UI Integration | 2-3 days | Medium | Planned |
| Phase 5: Testing | 2-3 days | High | Planned |
| Phase 6: Advanced Features | 3-5 days | Low | Planned |

### Risk Assessment

#### High Risk Items
1. **Memory Exhaustion**: May need to reduce buffer sizes or implement dynamic allocation
2. **CPU Overload**: Could require optimization or reduced sample rate
3. **Audio Dropouts**: May need buffer size adjustments or processing optimizations

#### Mitigation Strategies
- Implement compile-time voice count configuration
- Add runtime voice count limiting
- Provide fallback to 2-voice mode if issues arise

### Testing Checklist

#### Functional Tests
- [ ] All 4 voices produce sound simultaneously
- [ ] Parameter changes affect correct voices
- [ ] MIDI input routes to appropriate voices
- [ ] UI displays all 4 voices correctly

#### Performance Tests
- [ ] No audio dropouts at 48kHz with 4 voices
- [ ] CPU usage <80% with all features active
- [ ] Memory usage <90% of available RAM
- [ ] Responsive UI with all 4 voices active

#### Edge Cases
- [ ] Voice stealing behavior with overlapping notes
- [ ] Parameter updates during voice transitions
- [ ] MIDI overflow handling
- [ ] Power-on initialization with 4 voices

### Future Enhancements

#### Version 2.0 Features
- **User Voice Banks**: Save/load voice configurations
- **Modulation Matrix**: Advanced modulation routing
- **Arpeggiator**: Per-voice arpeggiation patterns
- **Sequencer Patterns**: Extended pattern memory for 4 voices

#### Hardware Considerations
- **External RAM**: Potential for external PSRAM for larger buffers
- **Audio Codec**: Higher quality audio output options
- **Control Surface**: Additional physical controls for 4-voice operation

### Conclusion
This plan provides a comprehensive roadmap for expanding the PicoMudrasSequencer to support 4 voices while maintaining performance and stability. The modular approach allows for incremental implementation and testing, with fallback options if resource constraints become problematic.

The success of this expansion depends heavily on careful memory management and processing optimization, given the resource constraints of the RP2040 platform. Regular testing and profiling throughout the implementation process will be crucial for identifying and addressing performance bottlenecks early.