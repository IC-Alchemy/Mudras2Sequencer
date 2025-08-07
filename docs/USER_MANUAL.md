# PicoMudrasSequencer User Manual

## Table of Contents
1. [Introduction](#introduction)
2. [Hardware Overview](#hardware-overview)
3. [Quick Start](#quick-start)
4. [Button Reference](#button-reference)
5. [Operating Modes](#operating-modes)
6. [Parameter System](#parameter-system)
7. [Voice System](#voice-system)
8. [Advanced Features](#advanced-features)
9. [LED Feedback System](#led-feedback-system)
10. [Musical Scales](#musical-scales)
11. [Troubleshooting](#troubleshooting)
12. [Technical Specifications](#technical-specifications)

## Introduction

The PicoMudrasSequencer is an advanced polyphonic step sequencer built on the Raspberry Pi Pico 2. It features dual-voice architecture, real-time parameter control, and expressive gesture-based input through distance sensing and magnetic encoder control.

### Key Features
- **Dual Voice Architecture**: Two independent sequencer voices with cross-modulation
- **Gesture Control**: Distance sensor for expressive parameter recording
- **Magnetic Encoder**: Velocity-sensitive real-time parameter control
- **LED Matrix Feedback**: 16-step visual feedback with color-coded voice indication
- **Polyphonic Capability**: Multiple notes per voice with dynamic allocation
- **Real-time Effects**: Stereo delay, state variable filter, overdrive, and wavefolder
- **Preset System**: Six voice presets per voice (Analog, Digital, Bass, Lead, Pad, Percussion)
- **Musical Scales**: Seven built-in scales with chromatic support
- **Polyrhythmic Sequencing**: Independent parameter lengths for complex rhythms

## Hardware Overview

### Physical Layout
```
┌─────────────────────────────────────────────────────────────────┐
│                        OLED Display                            │
├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤
│ P16 │ P17 │ P18 │ P19 │ P20 │ P21 │ P22 │ C23 │ C24 │ C25 │ C26 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ C27 │ C28 │ C29 │ C30 │ C31 │     │     │     │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │     │     │     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤ AS5600 Encoder │
│  8  │  9  │ 10  │ 11  │ 12  │ 13  │ 14  │ 15  │ & Distance     │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤ Sensor Area    │
│ 16  │ 17  │ 18  │ 19  │ 20  │ 21  │ 22  │ 23  │                │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤                │
│ 24  │ 25  │ 26  │ 27  │ 28  │ 29  │ 30  │ 31  │                │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴────────────────┘
```

### Components
- **32 Tactile Buttons**: Step matrix (32), with parameter/control functions on specific buttons
- **32 RGB LEDs**: Step visualization with voice color coding (4x8 matrix)
- **OLED Display**: Real-time parameter and status information
- **AS5600 Magnetic Encoder**: 12-bit contactless position sensor
- **Distance Sensor**: Gesture-based parameter recording
- **Raspberry Pi Pico 2**: Main processing unit with dual ARM Cortex-M33 cores

## Quick Start

### First Power-On
1. Connect power via USB-C
2. OLED displays startup information
3. LEDs perform initialization sequence
4. Default to Voice 1, Normal mode, stopped state

### Creating Your First Sequence
1. **Start Playback**: Press Play/Stop button (26)
2. **Add Steps**: Press step buttons (0-31) to toggle gates
3. **Add Melody**: Hold Note button (16) and move hand over sensor
4. **Adjust Volume**: Hold Velocity button (17) and move hand over sensor
5. **Fine-tune**: Use AS5600 encoder for precise adjustments

### Voice Switching
- Press Voice Switch button (24) to toggle between Voice 1 and Voice 2
- Voice 1: Green LEDs (steps 0-7 in rows 0-1)
- Voice 2: Cyan LEDs (steps 8-15 in rows 2-3)

## Button Reference

### Step Buttons (0-31)
**Location**: 4x8 LED matrix grid
**Functions**:
- **Short Press**: Toggle gate on/off for current voice
- **Long Press**: Select step for parameter editing
- **With Parameter Button**: Set parameter sequence length
- **In Slide Mode**: Toggle slide/legato on step
- **In Settings Mode**: Navigate menus and select options

**Voice Mapping**:
- Voice 1: Steps 0-7 (rows 0-1, green LEDs)
- Voice 2: Steps 8-15 (rows 2-3, cyan LEDs)

### Parameter Buttons (16-22)

#### Button 16 - Note
- **Range**: 0-21 (scale degrees)
- **Function**: Sets pitch within current scale
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 17 - Velocity
- **Range**: 0.0-1.0 (normalized)
- **Function**: Controls note volume/intensity
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 18 - Filter
- **Range**: 0.0-1.0 (normalized cutoff)
- **Function**: Controls filter cutoff frequency
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 19 - Attack
- **Range**: 0.0-1.0 seconds
- **Function**: Envelope attack time
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 20 - Decay
- **Range**: 0.0-1.0 seconds
- **Function**: Envelope decay time
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 21 - Octave
- **Range**: -1, 0, +1 (octave offset)
- **Function**: Octave transposition
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

#### Button 22 - Slide
- **Range**: 0.0-1.0 (slide time)
- **Function**: Controls slide/glide duration
- **Recording**: Hold + distance sensor movement
- **Real-time**: AS5600 encoder control

### Mode Buttons (23-25)

#### Button 23 - Delay
- **Short Press**: Toggle global delay effect
- **Function**: Enables/disables stereo delay
- **Parameters**: Controlled via AS5600 modes

#### Button 24 - Voice Switch
- **Short Press**: Toggle between Voice 1 and Voice 2
- **Function**: Changes active voice for editing
- **Visual**: LED colors change to indicate active voice

#### Button 25 - AS5600 Control
- **Short Press**: Cycle through parameter control modes
- **Double Press**: Recalibrate AS5600 sensor
- **Function**: Changes which parameter the encoder controls

### Control Buttons (26-31)

#### Button 26 - Play/Stop
- **Function**: Start/stop sequence playback
- **LED**: Green (playing) / Red (stopped)
- **Usage**: Press to toggle playback

#### Button 27 - Change Scale
- **Function**: Cycle through available musical scales
- **LED**: White when pressed
- **Usage**: Press to advance to next scale

#### Button 28 - Change Theme
- **Function**: Cycle through LED color themes
- **LED**: Various colors
- **Usage**: Press to change visual theme

#### Button 29 - Change Swing Pattern
- **Function**: Adjust timing swing/groove
- **LED**: Yellow when active
- **Usage**: Press to cycle swing patterns

#### Button 30 - Randomize Seq1
- **Function**: Randomize Voice 1 sequence
- **LED**: Purple flash
- **Usage**: Press to generate random pattern

#### Button 31 - Randomize Seq2
- **Function**: Randomize Voice 2 sequence
- **LED**: Purple flash
- **Usage**: Press to generate random pattern

## Operating Modes

### Normal Mode
**Default operating state for sequence creation and playback.**

**Behavior**:
- Step buttons toggle gates for active voice
- Parameter buttons enable recording mode
- AS5600 controls selected parameter
- LEDs show gate states and playhead position

**Workflow**:
1. Select voice with Voice Switch (24)
2. Create gate pattern with step buttons
3. Record parameters using parameter buttons + distance sensor
4. Fine-tune with AS5600 encoder

### Slide Mode
**Enables slide/legato editing for expressive sequences.**

**Entry**: Hold Parameter Button 22 (Slide)
**Exit**: Release Parameter Button 22

**Behavior**:
- Step buttons toggle slide on/off per step
- LEDs show slide states overlaid on gate pattern
- Slide creates smooth pitch transitions between notes
- Slide time controlled via AS5600 parameter mode

**Visual Feedback**:
- Slide-enabled steps show blended LED colors
- Non-slide steps maintain normal gate colors

### Settings Mode
**Comprehensive voice and system configuration.**

**Entry**: Long press Play/Stop (26) when sequencer is stopped
**Exit**: Start playback or press Play/Stop

**Main Menu**:
- **Step 0**: Voice 1 configuration
- **Step 1**: Voice 2 configuration

**Voice Configuration**:
1. Select voice (step 0 or 1)
2. Choose preset (steps 0-5):
   - **Step 0**: Analog - Warm, vintage character
   - **Step 1**: Digital - Clean, precise tones
   - **Step 2**: Bass - Low-frequency emphasis
   - **Step 3**: Lead - Cutting, melodic tones
   - **Step 4**: Pad - Atmospheric, sustained
   - **Step 5**: Percussion - Percussive, rhythmic

**Voice Parameters** (buttons 9-14):
- **Button 9**: Envelope on/off
- **Button 10**: Overdrive on/off
- **Button 11**: Wavefolder on/off
- **Button 12**: Filter mode (LP12/LP24/LP36/BP12/BP24)
- **Button 13**: Filter resonance (0.0-1.0)
- **Button 14**: Dalek effect on/off

### Parameter Length Mode
**Sets individual parameter sequence lengths for polyrhythmic patterns.**

**Entry**: Hold any parameter button (16-21)
**Exit**: Release parameter button

**Behavior**:
- Step buttons set sequence length (1-16 steps)
- LEDs show active steps for selected parameter
- Different parameters can have different lengths
- Creates complex polyrhythmic relationships

**Example**:
- Note parameter: 16 steps (full sequence)
- Velocity parameter: 12 steps (3/4 polyrhythm)
- Filter parameter: 8 steps (1/2 polyrhythm)

## Parameter System

### Parameter Recording
**Gesture-based parameter capture using distance sensor.**

**Process**:
1. Hold parameter button (16-21)
2. Move hand over distance sensor
3. Watch OLED for real-time value feedback
4. Release button to confirm recording
5. Parameter value stored to current step

**Tips**:
- Closer hand = higher values
- Smooth movements create gradual changes
- Quick gestures for dramatic effects
- Use with playback for real-time recording

### AS5600 Real-time Control
**Velocity-sensitive magnetic encoder for precise parameter adjustment.**

**Parameter Modes** (cycle with button 25):
1. **Velocity** - Note volume control
2. **Filter** - Filter cutoff control
3. **Attack** - Envelope attack control
4. **Decay** - Envelope decay control
5. **Note** - Note selection control
6. **Delay Time** - Global delay time
7. **Delay Feedback** - Delay feedback amount
8. **Slide Time** - Slide/glide duration

**Control Characteristics**:
- **Fast rotation**: Coarse adjustments (large steps)
- **Slow rotation**: Fine adjustments (small steps)
- **Velocity sensitivity**: Automatic scaling based on rotation speed
- **Cumulative tracking**: Maintains position across parameter switches

### Parameter Ranges and Defaults

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Note | 0-21 | 0 | Scale degree (root note) |
| Velocity | 0.0-1.0 | 0.8 | Note volume/intensity |
| Filter | 0.0-1.0 | 0.5 | Filter cutoff frequency |
| Attack | 0.0-1.0s | 0.1s | Envelope attack time |
| Decay | 0.0-1.0s | 0.3s | Envelope decay time |
| Octave | -1,0,+1 | 0 | Octave transposition |
| Gate Length | 0.0-1.0 | 0.8 | Note duration |
| Gate | 0/1 | 0 | Note on/off |
| Slide | 0/1 | 0 | Slide/legato enable |

## Voice System

### Voice Architecture
**Dual independent voices with cross-modulation capabilities.**

**Voice 1**:
- Steps 0-15 (LED rows 0-1)
- Green LED indication
- Full parameter control
- Cross-modulation source

**Voice 2**:
- Steps 16-31 (LED rows 2-3)
- Cyan LED indication
- Full parameter control
- Cross-modulation destination

**Clear Layout**:
- Voice assignment follows the traditional 16-step per voice layout
- Compact 4x8 matrix provides intuitive voice separation

### Voice Presets
**Six carefully crafted presets per voice optimized for different musical roles.**

#### Analog Preset
- **Character**: Warm, vintage synthesizer tones
- **Filter**: Low-pass with moderate resonance
- **Envelope**: Medium attack, long decay
- **Effects**: Subtle overdrive, no wavefolder
- **Use Cases**: Classic synth leads, warm pads

#### Digital Preset
- **Character**: Clean, precise digital tones
- **Filter**: High-pass or band-pass
- **Envelope**: Fast attack, short decay
- **Effects**: No overdrive, optional wavefolder
- **Use Cases**: Arpeggios, digital textures

#### Bass Preset
- **Character**: Low-frequency emphasis
- **Filter**: Low-pass with high resonance
- **Envelope**: Very fast attack, medium decay
- **Effects**: Moderate overdrive
- **Use Cases**: Bass lines, sub-bass

#### Lead Preset
- **Character**: Cutting, melodic tones
- **Filter**: Band-pass or high-pass
- **Envelope**: Medium attack, short decay
- **Effects**: High overdrive, wavefolder
- **Use Cases**: Melodic leads, solos

#### Pad Preset
- **Character**: Atmospheric, sustained tones
- **Filter**: Low-pass with low resonance
- **Envelope**: Slow attack, very long decay
- **Effects**: No overdrive, no wavefolder
- **Use Cases**: Ambient pads, backgrounds

#### Percussion Preset
- **Character**: Percussive, rhythmic sounds
- **Filter**: High-pass with variable resonance
- **Envelope**: Very fast attack, very short decay
- **Effects**: Variable overdrive, wavefolder
- **Use Cases**: Drum sounds, percussion

### Voice Parameters
**Detailed voice configuration options available in settings mode.**

#### Envelope Control (Button 9)
- **Function**: Enable/disable ADSR envelope
- **Off**: Direct gate control
- **On**: Shaped amplitude envelope

#### Overdrive (Button 10)
- **Function**: Harmonic saturation effect
- **Range**: Clean to heavy distortion
- **Character**: Warm tube-like saturation

#### Wavefolder (Button 11)
- **Function**: Non-linear waveshaping
- **Effect**: Adds harmonic complexity
- **Character**: Aggressive, metallic tones

#### Filter Mode (Button 12)
- **LP12**: 12dB/octave low-pass
- **LP24**: 24dB/octave low-pass
- **LP36**: 36dB/octave low-pass
- **BP12**: 12dB/octave band-pass
- **BP24**: 24dB/octave band-pass

#### Filter Resonance (Button 13)
- **Range**: 0.0-1.0
- **Function**: Filter feedback amount
- **Effect**: Emphasis at cutoff frequency

#### Dalek Effect (Button 14)
- **Function**: Ring modulation effect
- **Character**: Robotic, metallic modulation
- **Use**: Special effects, sci-fi sounds

## Advanced Features

### Polyrhythmic Sequencing
**Create complex rhythmic relationships using independent parameter lengths.**

**Concept**:
- Each parameter can have a different sequence length
- Parameters loop independently
- Creates evolving, non-repeating patterns

**Example Setup**:
1. Set Note parameter to 16 steps (full sequence)
2. Set Velocity parameter to 12 steps
3. Set Filter parameter to 8 steps
4. Result: Pattern repeats every 48 steps (LCM of 16, 12, 8)

**Applications**:
- Evolving melodic patterns
- Dynamic rhythm variations
- Generative composition techniques

### Cross-Voice Modulation
**Voice 1 can modulate Voice 2 parameters for complex interactions.**

**Modulation Sources** (Voice 1):
- Envelope output
- Filter output
- LFO (if implemented)

**Modulation Destinations** (Voice 2):
- Filter cutoff
- Amplitude
- Pitch (vibrato)

### Real-time Effects Processing

#### Stereo Delay
- **Control**: AS5600 parameter modes 6-7
- **Parameters**: Delay time, feedback amount
- **Character**: Stereo ping-pong delay
- **Range**: 10ms to 2 seconds

#### State Variable Filter
- **Types**: Low-pass, high-pass, band-pass
- **Slopes**: 12dB, 24dB, 36dB per octave
- **Resonance**: 0-95% feedback
- **Tracking**: Keyboard tracking available

#### Overdrive
- **Type**: Soft clipping saturation
- **Character**: Tube-like warmth
- **Range**: Clean to heavy distortion
- **Per-voice**: Independent control

#### Wavefolder
- **Type**: Non-linear waveshaping
- **Effect**: Harmonic multiplication
- **Character**: Aggressive, metallic
- **Per-voice**: Independent control

### Dynamic Voice Allocation
**Intelligent note assignment for polyphonic playback.**

**Algorithm**:
1. Check for available voice slots
2. Assign new notes to free slots
3. Use voice stealing for overflow
4. Prioritize recent notes

**Polyphonic Modes**:
- **Monophonic**: One note per voice
- **Polyphonic**: Multiple notes per voice
- **Unison**: All voices play same note

## LED Feedback System

### Color Coding
**Comprehensive visual feedback using RGB LEDs.**

#### Voice Colors
- **Voice 1**: Green spectrum
  - Bright green: Gate on
  - Dim green: Gate off
  - Orange accent: Current playhead
- **Voice 2**: Cyan spectrum
  - Bright cyan: Gate on
  - Dim cyan: Gate off
  - Orange accent: Current playhead

#### Mode Indicators
- **Normal Mode**: Standard voice colors
- **Slide Mode**: Blended colors for slide steps
- **Settings Mode**: Pulsing selection indicators
- **Parameter Length**: Parameter-specific colors
- **Edit Mode**: Blue tint overlay

#### Special States
- **Idle**: Blue breathing effect
- **Recording**: Pulsing parameter color
- **Error**: Red flash indication
- **Calibration**: White sweep pattern

### LED Themes
**Multiple color palettes for different preferences and environments.**

**Theme 1 - Standard**:
- High contrast colors
- Bright, vibrant palette
- Optimal for normal lighting

**Theme 2 - Warm**:
- Warm color temperature
- Reduced blue content
- Comfortable for extended use

**Theme 3 - Cool**:
- Cool color temperature
- Blue/cyan emphasis
- Modern, digital aesthetic

**Theme 4 - Monochrome**:
- Single hue variations
- Brightness-based indication
- Minimal distraction

### Brightness Control
**Automatic and manual brightness adjustment.**

**Auto Mode**:
- Ambient light sensing
- Automatic adjustment
- Power saving in dark environments

**Manual Mode**:
- User-controlled brightness
- Consistent appearance
- Performance optimization

## Musical Scales

### Scale System
**Seven built-in scales covering various musical styles and traditions.**

#### 1. Mixolydian (Default)
- **Intervals**: W-W-H-W-W-H-W
- **Character**: Major with flattened 7th
- **Use**: Rock, blues, folk
- **Notes** (C): C-D-E-F-G-A-Bb

#### 2. Pentatonic Minor
- **Intervals**: m3-W-W-m3-W
- **Character**: Five-note minor scale
- **Use**: Blues, rock, world music
- **Notes** (A): A-C-D-E-G

#### 3. Dorian
- **Intervals**: W-H-W-W-W-H-W
- **Character**: Minor with raised 6th
- **Use**: Jazz, Celtic, modal
- **Notes** (D): D-E-F-G-A-B-C

#### 4. Phrygian Dominant
- **Intervals**: H-A2-H-W-H-W-W
- **Character**: Exotic with augmented 2nd
- **Use**: Spanish, Middle Eastern
- **Notes** (E): E-F-G#-A-B-C-D

#### 5. Lydian Dominant
- **Intervals**: W-W-W-H-W-H-W
- **Character**: Major with raised 4th, flat 7th
- **Use**: Jazz, fusion
- **Notes** (F): F-G-A-B-C-D-Eb

#### 6. Wholetone
- **Intervals**: W-W-W-W-W-W
- **Character**: All whole steps
- **Use**: Impressionist, ambient
- **Notes** (C): C-D-E-F#-G#-A#

#### 7. Chromatic
- **Intervals**: H-H-H-H-H-H-H-H-H-H-H-H
- **Character**: All twelve semitones
- **Use**: Atonal, experimental
- **Notes**: All 12 chromatic notes

### Scale Selection
**Cycle through scales using Scale button (27).**

**Process**:
1. Press Scale button (27)
2. OLED displays current scale name
3. All existing note parameters reinterpreted
4. New recordings use new scale

**Global Effect**:
- Affects both voices simultaneously
- Existing sequences adapt to new scale
- Maintains relative interval relationships

## Troubleshooting

### Common Issues

#### No Sound Output
**Symptoms**: LEDs active, no audio
**Causes**:
- Gates disabled (velocity = 0)
- Filter completely closed
- Audio output disconnected
- Voice envelope disabled

**Solutions**:
1. Check gate states (LED brightness)
2. Adjust velocity parameter
3. Open filter cutoff
4. Verify audio connections
5. Enable envelope in settings

#### Erratic Parameter Control
**Symptoms**: Jumpy or unresponsive AS5600
**Causes**:
- Magnetic interference
- Calibration drift
- Physical obstruction

**Solutions**:
1. Remove magnetic objects
2. Recalibrate (double-press button 25)
3. Check for physical obstructions
4. Power cycle device

#### Wrong Voice Active
**Symptoms**: Editing unexpected voice
**Causes**:
- Voice switch confusion
- LED color misinterpretation

**Solutions**:
1. Check LED colors (green=V1, cyan=V2)
2. Press Voice Switch (24) to toggle
3. Verify OLED voice indicator

#### Timing Issues
**Symptoms**: Irregular rhythm, sync problems
**Causes**:
- Shuffle pattern active
- Different parameter lengths
- Clock drift

**Solutions**:
1. Check shuffle setting (button 29)
2. Verify parameter lengths
3. Reset to default timing
4. Power cycle for clock reset

#### LED Problems
**Symptoms**: Dim, wrong colors, or no LEDs
**Causes**:
- Power supply issues
- LED driver problems
- Theme settings

**Solutions**:
1. Check power supply capacity
2. Try different theme (button 28)
3. Adjust brightness settings
4. Power cycle device

### Calibration Procedures

#### AS5600 Calibration
**When**: Erratic encoder behavior
**Process**:
1. Double-press AS5600 Control (25)
2. Follow OLED instructions
3. Rotate encoder through full range
4. Confirm calibration complete

#### Distance Sensor Calibration
**When**: Inconsistent gesture response
**Process**:
1. Access settings mode
2. Navigate to sensor calibration
3. Follow min/max range procedure
4. Test with parameter recording

### Reset Procedures

#### Parameter Reset
**Function**: Clear all parameter data
**Method**: Long press Randomize buttons (30/31)
**Effect**: Returns parameters to defaults

#### Voice Reset
**Function**: Reset voice configuration
**Method**: Reapply preset in settings mode
**Effect**: Restores preset defaults

#### Factory Reset
**Function**: Complete system reset
**Method**: Hold multiple buttons during power-on
**Effect**: All settings return to factory defaults

### Error Codes

#### E01 - Sensor Error
**Meaning**: Distance sensor communication failure
**Action**: Check connections, power cycle

#### E02 - AS5600 Error
**Meaning**: Magnetic encoder communication failure
**Action**: Check magnet position, recalibrate

#### E03 - Memory Error
**Meaning**: Parameter storage failure
**Action**: Power cycle, check for corruption

#### E04 - LED Error
**Meaning**: LED matrix communication failure
**Action**: Check connections, reduce brightness

## Technical Specifications

### Hardware
- **Processor**: Raspberry Pi Pico 2 (RP2350)
- **CPU**: Dual ARM Cortex-M33 @ 150MHz
- **Memory**: 520KB SRAM, 4MB Flash
- **I/O**: 32 tactile buttons, 16 RGB LEDs
- **Display**: 128x64 OLED (I2C)
- **Sensors**: AS5600 magnetic encoder, distance sensor
- **Audio**: I2S output, 16-bit/44.1kHz
- **Power**: USB-C, 5V/1A minimum

### Software
- **Framework**: Arduino IDE compatible
- **Real-time**: Dual-core processing
- **Audio Engine**: Custom DSP implementation
- **Parameter Resolution**: 12-bit (4096 steps)
- **Timing Resolution**: 1ms accuracy
- **Storage**: Non-volatile parameter memory

### Performance
- **Polyphony**: Up to 8 voices per channel
- **Latency**: <5ms audio latency
- **Parameter Update**: 1kHz rate
- **LED Refresh**: 60Hz
- **OLED Refresh**: 30Hz
- **Button Scan**: 1kHz

### Connectivity
- **Audio Output**: 3.5mm stereo jack
- **MIDI**: USB MIDI class compliant
- **Power**: USB-C connector
- **Programming**: USB bootloader
- **Expansion**: GPIO header (optional)

### Environmental
- **Operating Temperature**: 0°C to 40°C
- **Storage Temperature**: -20°C to 60°C
- **Humidity**: 10% to 90% non-condensing
- **Dimensions**: 150mm x 100mm x 25mm
- **Weight**: 200g

---

**For additional support, updates, and community resources, visit the PicoMudrasSequencer project repository.**