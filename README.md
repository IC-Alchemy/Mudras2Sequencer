# PicoMudrasSequencer

**A Dual-Voice Polymetric Step Sequencer for Raspberry Pi Pico2**

*Status: Functional and actively developed*



## Philosophy

The interface prioritizes immediate tactile control over menu diving. Hold a parameter button and move your hand over the distance sensor to record values in real-time. The AS5600 magnetic encoder provides continuous parameter modification with velocity-sensitive scaling. Every interaction provides instant visual feedback through the 8×8 LED matrix.

## Core Features

### Polymetric Parameter Sequencing
- **Independent Track Lengths**: Each parameter can have its own step count (2-64 steps)
- **Complex Pattern Evolution**: Patterns repeat only after reaching the LCM of all track lengths
- **Real-Time Parameter Recording**: Hold parameter buttons (16-21) to record sensor values instantly
- **Dual Voice Architecture**: Two independent sequencers for layered compositions

- **VL53L1X Distance Sensor**: Hand gestures control parameter values with millimeter precision
- **AS5600 Magnetic Encoder**: Continuous parameter modification with velocity-sensitive scaling (±0.8 range)
- **32-Button Capacitive Matrix**: MPR121-based touch interface for step selection and mode switching
- **Visual Feedback**: 8×8 WS2812B LED matrix provides real-time pattern visualization

## Hardware Requirements

### Core Components
- **Raspberry Pi Pico2**: Dual-core RP2350 microcontroller with floating-point DSP
- **Audio Codec**: I2S-compatible DAC for 24-bit/48kHz stereo output
- **VL53L1X LIDAR Sensor**: Time-of-flight sensor for gesture control (400mm range)
- **AS5600 Magnetic Encoder**: 12-bit contactless rotary encoder for parameter control
- **MPR121 Capacitive Touch Controller**: I2C interface for 32-button matrix
- **WS2812B LED Matrix**: 8×8 addressable RGB matrix for visual feedback

## Getting Started

### Quick Start Guide

1. **Hardware Setup**
   - Connect VL53L1X distance sensor to I2C pins (SDA/SCL)
   - Wire AS5600 encoder to I2C bus (shared with distance sensor)
   - Connect MPR121 to I2C for button matrix
   - Attach WS2812B LED matrix to designated data pin
   - Connect I2S audio codec for stereo output


3. **Basic Operation**
   - Power on device - LED matrix shows initialization pattern
   - Press button 26 to start/stop the sequencer clock
   - Hold parameter buttons (16-21) and move hand over distance sensor to record values in realtime across all steps
   - Long press a step button (0-15) to select an individual step for more traditional editing with the magnetic encoder, or distance sensor.

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

4. **Polymetric Recording**: Each parameter records to its own current step position, enabling independent track lengths

### AS5600 Encoder Control

The magnetic encoder provides continuous parameter modification with velocity-sensitive scaling:

- **Slow Rotation**: Fine control with 2× scaling for precise adjustments
- **Fast Rotation**: Coarse control with up to 16× scaling for rapid changes
- **Parameter Selection**: Press button 25 to cycle through Velocity, Filter, Attack, and Decay
- **Reset Function**: Double-press button 25 to reset encoder base values
- **Visual Feedback**: LED flash speed increases as values approach ±0.8 boundaries

## Key Libraries and Dependencies

- **[Mutable Instruments](https://github.com/pichenettes/eurorack)**
- **[FastLED](https://github.com/FastLED/FastLED)**: High-performance LED control library for WS2812B matrices
- **[UClock](https://github.com/midilab/uClock)**: Precision timing library for MIDI synchronization
- **[Adafruit MPR121](https://github.com/adafruit/Adafruit_MPR121)**: Capacitive touch sensor library for button matrix
- **[Melopero VL53L1X](https://github.com/melopero/VL53L1X)**: Time-of-flight distance sensor driver
- **[RobTillaart AS5600](https://github.com/RobTillaart/AS5600)**: Magnetic rotary encoder library

