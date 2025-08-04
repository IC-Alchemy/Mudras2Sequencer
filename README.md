# PicoMudrasSequencer

**A Dual-Voice Polyrhythmic Step Sequencer for Raspberry Pi Pico2**

*Status: Functional and actively developed*

The PicoMudrasSequencer is an expressive, gesture-controlled step sequencer designed for electronic music performance and composition. Built around the Raspberry Pi Pico2's dual-core architecture, it combines real-time audio synthesis with intuitive parameter control through distance sensors and magnetic encoders.

## Philosophy

This sequencer embraces the modular synthesis approach to pattern creation: each parameter (Note, Velocity, Filter, Attack, Decay, Octave) operates as an independent track with its own length and timing. This polyrhythmic architecture enables complex, evolving patterns that repeat only after reaching the least common multiple of all track lengths—creating compositions that can evolve over hundreds of steps before returning to their starting point.

The interface prioritizes immediate tactile control over menu diving. Hold a parameter button and move your hand over the distance sensor to record values in real-time. The AS5600 magnetic encoder provides continuous parameter modification with velocity-sensitive scaling. Every interaction provides instant visual feedback through the 8×8 LED matrix.

## Core Features

### Polyrhythmic Parameter Sequencing
- **Independent Track Lengths**: Each parameter can have its own step count (2-64 steps)
- **Complex Pattern Evolution**: Patterns repeat only after reaching the LCM of all track lengths
- **Real-Time Parameter Recording**: Hold parameter buttons (16-21) to record sensor values instantly
- **Dual Voice Architecture**: Two independent sequencers for layered compositions

### Gesture-Based Control
- **VL53L1X Distance Sensor**: Hand gestures control parameter values with millimeter precision
- **AS5600 Magnetic Encoder**: Continuous parameter modification with velocity-sensitive scaling (±0.8 range)
- **32-Button Capacitive Matrix**: MPR121-based touch interface for step selection and mode switching
- **Visual Feedback**: 8×8 WS2812B LED matrix provides real-time pattern visualization

### Audio Synthesis
- **DaisySP Integration**: Professional-quality oscillators, filters, and envelopes
- **Dual-Core Architecture**: Core0 dedicated to audio synthesis, Core1 handles sequencing and UI
- **I2S Audio Output**: 24-bit/48kHz audio with stereo processing
- **MIDI Output**: USB MIDI for external synthesizer control with real-time CC transmission

## Architecture

### Dual-Core Design

The PicoMudrasSequencer leverages the Raspberry Pi Pico2's dual-core ARM Cortex-M33 architecture for optimal real-time performance:

**Core0 (Audio Synthesis)**
- Dedicated audio buffer processing at 48kHz sample rate
- DaisySP oscillators, filters, and envelope generators
- I2S audio output with stereo processing
- Delay effects and signal routing
- Thread-safe communication via volatile VoiceState structures

**Core1 (Sequencer & UI)**
- Sequencer logic and parameter management
- Sensor data acquisition and processing
- LED matrix updates and visual feedback
- MIDI output and USB communication
- Button matrix scanning and event handling

### Data Flow

```
Sensor Input → normalize → setStepParameterValue() → ParameterTrack →
processStep() → VoiceState update → memcpy to volatile global →
Core0 audio synthesis → I2S output
```

This architecture ensures that audio synthesis never blocks on sensor reads or LED updates, maintaining consistent audio quality even during intensive UI interactions.

## MIDI Implementation

### MIDI Note Output

The PicoMudrasSequencer provides comprehensive MIDI output for external synthesizer control:

- **USB MIDI Interface**: Standard USB MIDI class-compliant output
- **Dual Voice Support**: Independent MIDI channels for Voice 1 and Voice 2
- **Proper Note Lifecycle**: Accurate note-on/off pairing with gate timing synchronization
- **Octave Offset Support**: Full octave range with proper MIDI note mapping
- **Velocity Control**: 7-bit velocity transmission (0-127)

### MIDI Continuous Controller (CC) Output

**Real-time parameter control via MIDI CC messages for seamless DAW integration:**

#### CC Number Mapping

| Parameter | Voice 1 | Voice 2 | Description |
|-----------|---------|---------|-------------|
| **Filter** | CC74 | CC78 | Filter cutoff frequency (standard MIDI CC) |
| **Attack** | CC73 | CC77 | Envelope attack time |
| **Decay** | CC72 | CC76 | Envelope decay time |
| **Octave** | CC71 | CC75 | Octave offset (-12 to +12 semitones) |

#### Key Features

- **Change Detection**: Only transmits CC when parameter values actually change (prevents MIDI spam)
- **Rate Limiting**: Maximum 100 Hz transmission rate for optimal MIDI buffer management
- **Value Scaling**: Automatic conversion from internal 0.0-1.0 range to MIDI 0-127 range
- **Real-time Transmission**: CC messages sent immediately during:
  - Step sequencer playback
  - Real-time parameter recording (distance sensor)
  - AS5600 encoder parameter modifications
  - Manual step editing

#### DAW Integration Examples

**Ableton Live:**
```
1. Create MIDI track
2. Set input to "PicoMudrasSequencer"
3. Map CC74/78 to filter cutoff on your synthesizer
4. Map CC73/77 to filter envelope attack
5. Real-time parameter changes now control your DAW instruments
```

**Logic Pro:**
```
1. Create External Instrument track
2. Set MIDI input to "PicoMudrasSequencer"
3. Use MIDI Learn to map CC numbers to synthesizer parameters
4. Enable "Auto demix by channel if multitrack" for dual voice separation
```

#### Technical Specifications

- **MIDI Channel**: All CC messages transmitted on Channel 1
- **Resolution**: 7-bit (0-127) for maximum compatibility
- **Latency**: <10ms from parameter change to CC transmission
- **Thread Safety**: CC transmission occurs on Core1 with proper synchronization

## Hardware Requirements

### Core Components
- **Raspberry Pi Pico2**: Dual-core RP2350 microcontroller with floating-point DSP
- **Audio Codec**: I2S-compatible DAC for 24-bit/48kHz stereo output
- **VL53L1X Distance Sensor**: Time-of-flight sensor for gesture control (400mm range)
- **AS5600 Magnetic Encoder**: 12-bit contactless rotary encoder for parameter control
- **MPR121 Capacitive Touch Controller**: I2C interface for 32-button matrix
- **WS2812B LED Matrix**: 8×8 addressable RGB matrix for visual feedback

### Recommended Specifications
- **Power Supply**: 5V/2A minimum for stable operation with LEDs
- **Audio Output**: Line-level stereo output, impedance-matched for mixing consoles
- **Enclosure**: Tactile button matrix with 4×8 grid layout
- **Connectivity**: USB-C for power, programming, and MIDI communication

## Getting Started

### Quick Start Guide

1. **Hardware Setup**
   - Connect VL53L1X distance sensor to I2C pins (SDA/SCL)
   - Wire AS5600 encoder to I2C bus (shared with distance sensor)
   - Connect MPR121 to I2C for button matrix
   - Attach WS2812B LED matrix to designated data pin
   - Connect I2S audio codec for stereo output

2. **Software Installation**
   - Install Arduino IDE with Raspberry Pi Pico board support
   - Add required libraries via Library Manager
   - Clone this repository and open PicoMudrasSequencer.ino
   - Select "Raspberry Pi Pico2" as target board
   - Upload firmware to device

3. **Basic Operation**
   - Power on device - LED matrix shows initialization pattern
   - Press button 26 to start/stop the sequencer clock
   - Hold parameter buttons (16-21) and move hand over distance sensor to record values
   - Use AS5600 encoder to modify parameters in real-time
   - Press step buttons (0-15) to select individual steps for editing

### Parameter Recording Workflow

The sequencer uses a real-time parameter recording system that provides immediate audio feedback:

1. **Hold Parameter Button**: Press and hold any parameter button (16-21)
   - Button 16: Note values
   - Button 17: Velocity levels
   - Button 18: Filter frequency
   - Button 19: Attack time
   - Button 20: Decay time
   - Button 21: Octave offset

2. **Move Hand Over Sensor**: Distance sensor readings are normalized and mapped to parameter ranges

3. **Instant Feedback**: Parameter values are immediately applied to the current step and heard in the audio output

4. **Polyrhythmic Recording**: Each parameter records to its own current step position, enabling independent track lengths

### AS5600 Encoder Control

The magnetic encoder provides continuous parameter modification with velocity-sensitive scaling:

- **Slow Rotation**: Fine control with 2× scaling for precise adjustments
- **Fast Rotation**: Coarse control with up to 16× scaling for rapid changes
- **Parameter Selection**: Press button 25 to cycle through Velocity, Filter, Attack, and Decay
- **Reset Function**: Double-press button 25 to reset encoder base values
- **Visual Feedback**: LED flash speed increases as values approach ±0.8 boundaries

## Key Libraries and Dependencies

- **[DaisySP](https://github.com/electro-smith/DaisySP)**: Professional DSP library for oscillators, filters, and envelopes
- **[FastLED](https://github.com/FastLED/FastLED)**: High-performance LED control library for WS2812B matrices
- **[UClock](https://github.com/midilab/uClock)**: Precision timing library for MIDI synchronization
- **[VL53L1X](https://github.com/pololu/vl53l1x-arduino)**: Time-of-flight distance sensor library
- **[AS5600](https://github.com/Seeed-Studio/Seeed_Arduino_AS5600)**: Magnetic rotary encoder library
- **[Adafruit_MPR121](https://github.com/adafruit/Adafruit_MPR121)**: Capacitive touch sensor controller
- **[Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library)**: OLED display graphics library

## Polyrhythmic Sequencing Examples

### Basic 3:4 Polyrhythm
```
Voice 1: Note track = 3 steps, Velocity track = 4 steps
Pattern repeats every 12 steps (LCM of 3 and 4)
Creates syncopated rhythm with evolving velocity accents
```

### Complex 5:7:9 Pattern
```
Note: 5 steps, Velocity: 7 steps, Filter: 9 steps
Total cycle length: 315 steps (LCM of 5,7,9)
Pattern evolves over 6+ minutes at 120 BPM
```

### Ambient Texture Creation
```
All tracks set to prime numbers (7,11,13,17,19,23)
Creates non-repeating patterns for 100,000+ steps
Perfect for generative ambient music
```

## Advanced Features

### VoiceManager System
The new VoiceManager architecture provides:
- **Dynamic Voice Allocation**: Up to 4 voices with preset management
- **Preset System**: Save and recall complete voice configurations
- **Polyphonic Mode**: Play multiple notes simultaneously
- **Cross-Voice Modulation**: Voices can modulate each other

### Real-Time Effects Processing
- **Stereo Delay**: Tempo-synced delay with feedback control
- **State Variable Filter**: Low-pass, high-pass, and band-pass modes

- **Overdrive & Wavefolder**: Analog-style saturation and wave shaping

### Memory Architecture
- **Static Allocation**: All memory pre-allocated for real-time guarantees
- **Thread-Safe Communication**: Lock-free queues between cores
- **Efficient Data Structures**: Optimized for 32-bit ARM architecture
- **Minimal Heap Usage**: Prevents memory fragmentation

## Development and Customization

### Project Structure
```
src/
├── sequencer/          # Core polyrhythmic sequencer
├── voice/             # Voice management and synthesis
├── sensors/           # VL53L1X and AS5600 drivers
├── LEDMatrix/         # WS2812B LED matrix control
├── OLED/              # OLED display drivers
├── ui/                # User interface components
├── dsp/               # Digital signal processing
├── midi/              # MIDI implementation
└── audio/             # Audio I/O and processing
```

### Adding New Parameters
1. **Define Parameter ID** in `SequencerDefs.h`
2. **Update ParameterManager** to handle new track
3. **Add MIDI CC mapping** in MIDI implementation
4. **Create LED visualization** in LEDMatrix module
5. **Update UI handlers** for button matrix

### Extending Sensor Integration
- **New Sensor Types**: Follow existing pattern in `sensors/` directory
- **Calibration System**: Implement auto-calibration routines
- **Gesture Recognition**: Add gesture detection algorithms
- **Velocity Curves**: Customizable response curves per sensor

### Performance Optimization
- **DSP Optimization**: Use fixed-point math where possible
- **Memory Layout**: Align data structures for cache efficiency
- **Interrupt Priorities**: Optimize for minimal audio latency
- **Profiling Tools**: Built-in performance monitoring

## Performance Specifications

| Metric | Value | Notes |
|--------|--------|--------|
| **Audio Latency** | <5ms | End-to-end from sensor to audio |
| **MIDI Latency** | <10ms | From parameter change to CC transmission |
| **LED Update Rate** | 60Hz | Smooth visual feedback |
| **Sensor Sample Rate** | 100Hz | Responsive gesture control |
| **Maximum Polyphony** | 4 voices | With full effects chain |
| **Memory Usage** | ~80% | 256KB total available |
| **CPU Usage Core0** | ~60% | Audio synthesis and effects |
| **CPU Usage Core1** | ~40% | Sequencer and UI processing |

## Troubleshooting

### Common Issues

**Audio Dropouts**
- Check power supply (minimum 5V/2A)
- Verify I2S connections
- Reduce LED brightness if using many pixels

**MIDI Not Working**
- Ensure USB cable is data-capable (not just charging)
- Check MIDI channel settings in DAW
- Verify CC mapping matches documentation

**Sensor Drift**
- Recalibrate distance sensor using button 24
- Check for interference from bright lights
- Ensure magnetic encoder has proper magnet spacing

**LED Matrix Issues**
- Verify 5V power supply for LEDs
- Check data pin connections
- Ensure proper grounding

### Debug Mode
Hold button 23 during startup to enter diagnostic mode:
- LED matrix shows sensor readings
- OLED displays performance metrics
- Audio outputs test tones

## Future Development

### Planned Features
- **SD Card Support**: Save/load patterns and presets
- **USB Audio**: Direct audio interface mode
- **Wireless MIDI**: Bluetooth LE MIDI support
- **CV/Gate Output**: Analog control voltage generation
- **Euclidean Patterns**: Built-in euclidean rhythm generator

### Contributing
We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for:
- Code style guidelines
- Hardware extension documentation
- Testing procedures
- Pull request process

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **DaisySP Team**: For the excellent DSP library
- **FastLED Community**: For LED matrix inspiration
- **Polyrhythm Research**: Inspired by the work of composer György Ligeti
- **Gesture Control**: Based on research from IRCAM and CNMAT

---

**Made with ❤️ for the electronic music community**

For questions, issues, or contributions, please visit our [GitHub repository](https://github.com/yourusername/PicoMudrasSequencer).
- **[Adafruit MPR121](https://github.com/adafruit/Adafruit_MPR121)**: Capacitive touch sensor library for button matrix
- **[Melopero VL53L1X](https://github.com/melopero/VL53L1X)**: Time-of-flight distance sensor driver
- **[RobTillaart AS5600](https://github.com/RobTillaart/AS5600)**: Magnetic rotary encoder library

## Polyrhythmic Sequencing Examples

### Basic Polyrhythmic Setup

Create evolving patterns by setting different track lengths:

```cpp
// Configure different track lengths for polyrhythmic behavior
seq1.setParameterStepCount(ParamId::Gate, 4);     // 4-beat rhythm pattern
seq1.setParameterStepCount(ParamId::Note, 3);     // 3-note melodic cycle
seq1.setParameterStepCount(ParamId::Velocity, 5); // 5-level dynamic cycle
seq1.setParameterStepCount(ParamId::Filter, 2);   // 2-state filter cycle

// Pattern repeats every LCM(4,3,5,2) = 60 steps
// Creates 60 unique combinations before returning to start
```

### Advanced Polyrhythmic Patterns

```cpp
// Complex evolving pattern with 7 parameters
seq1.setParameterStepCount(ParamId::Gate, 8);       // Rhythm: 8 beats
seq1.setParameterStepCount(ParamId::Note, 7);       // Melody: 7 notes
seq1.setParameterStepCount(ParamId::Velocity, 5);   // Dynamics: 5 levels
seq1.setParameterStepCount(ParamId::Filter, 3);     // Timbre: 3 settings
seq1.setParameterStepCount(ParamId::Attack, 4);     // Attack: 4 variations
seq1.setParameterStepCount(ParamId::Decay, 6);      // Decay: 6 variations
seq1.setParameterStepCount(ParamId::Octave, 2);     // Octave: 2 ranges

// Pattern repeats every LCM(8,7,5,3,4,6,2) = 840 steps!
// Creates incredibly complex, slowly evolving compositions
```

### Musical Applications

- **Minimalist Compositions**: Use small prime numbers (2,3,5,7) for subtle evolution
- **Complex Rhythms**: Combine different rhythm lengths for polyrhythmic percussion
- **Evolving Melodies**: Use different note and octave cycle lengths
- **Dynamic Expression**: Vary velocity, attack, and decay independently
- **Timbral Evolution**: Use filter and other effect parameters with unique cycles

## Development and Customization

### Project Structure

```
PicoMudrasSequencer/
├── PicoMudrasSequencer.ino    # Main application file
├── src/
│   ├── sequencer/             # Core sequencing logic
│   ├── audio/                 # Audio I/O and buffering
│   ├── dsp/                   # Digital signal processing
│   ├── sensors/               # Distance and encoder sensors
│   ├── matrix/                # Button matrix handling
│   ├── LEDMatrix/             # LED control and feedback
│   ├── ui/                    # User interface event handling
│   └── memory/                # Memory management utilities
├── README.md                  # This file
└── PROGRAMMERS_MANUAL.md      # Technical documentation
```

### Adding New Parameters

1. Add new entry to `ParamId` enum in `SequencerDefs.h`
2. Define parameter metadata in `CORE_PARAMETERS` array
3. Update `VoiceState` structure if needed for audio synthesis
4. Integrate parameter in sequencer processing logic

### Extending Sensor Integration

The modular sensor architecture supports additional input devices:

- Implement sensor driver in `src/sensors/`
- Add sensor update calls to Core1 main loop
- Map sensor values to parameter ranges using existing normalization functions
- Integrate with LED feedback system for visual confirmation

## Contributing and Support

This project is fully open-source and welcomes contributions from the electronic music and embedded systems communities. Whether you're interested in adding new DSP effects, implementing additional sensor types, or improving the user interface, your contributions help advance expressive music technology.

### Development Priorities

- **Hardware MIDI Output**: DIN-5 MIDI connector for hardware synthesizer control
- **Analog Clock Input**: External clock synchronization for studio integration
- **Advanced Randomization**: Intelligent pattern generation with musical constraints
- **Preset Management**: Save and recall complete sequencer configurations
- **Performance Modes**: Specialized interfaces for live performance

---

**For detailed technical documentation, see [PROGRAMMERS_MANUAL.md](PROGRAMMERS_MANUAL.md)**

**For module-specific documentation, explore the README files in each `src/` subdirectory**
