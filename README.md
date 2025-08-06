# PicoMudrasSequencer

**A Dual-Voice Polyrhythmic Step Sequencer for Raspberry Pi Pico2**

*Status: Functional and actively developed*

## Core Features

### Polymetric rhythmic Parameter Sequencing
- **Independent Track Lengths**: Each parameter can have its own step count (2-64 steps)
- **Real-Time Parameter Recording**: Hold parameter buttons (16-21) to record sensor values instantly

### Gesture-Based Control
- **VL53L1X Distance Sensor**:Hand distance above sensor is written to sequence per step 
- **AS5600 Magnetic Encoder**: Continuous parameter modification with velocity-sensitive scaling (±0.8 range)
- **32-Button Capacitive Matrix**: MPR121-based touch interface for step selection and mode switching
- **Visual Feedback**: 8×8 WS2812B LED matrix provides real-time pattern visualization


#### CC Number Mapping

| Parameter | Voice 1 | Voice 2 | Description |
|-----------|---------|---------|-------------|
| **Filter** | CC74 | CC78 | Filter cutoff frequency (standard MIDI CC) |
| **Attack** | CC73 | CC77 | Envelope attack time |
| **Decay** | CC72 | CC76 | Envelope decay time |
| **Octave** | CC71 | CC75 | Octave offset (-12 to +12 semitones) |

## Getting Started

### Parameter Recording Workflow
#### Real Time Recording 
The sequencer uses a real-time parameter records values to each step as the sequencer plays.

1. **Hold Parameter Button**: Press and hold any parameter button (16-21)
   - Button 16: Note values
   - Button 17: Velocity levels
   - Button 18: Filter frequency
   - Button 19: Attack time
   - Button 20: Decay time
   - Button 21: Octave offset

2. **Move Hand Over Sensor**: Distance sensor readings are normalized and mapped to parameter ranges
3. **Instant Feedback**: Parameter values are immediately applied to the current step and heard in the audio output
4. **Polymetric Recording**: Each parameter records to its own current step position, enabling independent track lengths

#### Per Step Recording
The AS5600 magnetic encoder enables precise per-step parameter editing:

1. **Select Step**: Press and hold any step button (1-16) to edit

2. **Choose Parameter**: Press Any Parameter Button (16-21) to select parameter (Velocity/Filter/Attack/Decay)
3. **Adjust Value**: Rotate encoder clockwise/counterclockwise
   - Slow rotation: Fine 2× scaling
   - Fast rotation: Coarse up to 16× scaling




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
