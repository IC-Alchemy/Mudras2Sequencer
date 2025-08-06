# PicoMudrasSequencer User Manual Plan

## Table of Contents

### 1. Quick Start Guide (1 page)
- Button Layout Overview
- Basic Operation in Each Mode
- Essential Controls Reference

### 2. Hardware Overview
- Physical Layout
- LED Matrix (8x4 grid)
- Control LEDs
- AS5600 Rotary Encoder
- Distance Sensor
- OLED Display

### 3. Basic Operation
- Power On/Setup
- Playing Sequences
- Recording Parameters
- Voice Selection

### 4. Button Reference
- Step Buttons (0-15)
- Parameter Buttons (16-21)
- Mode/Control Buttons (22-31)
- Press Types (Short/Long)

### 5. Operating Modes
- Normal Sequencing Mode
- Slide Mode
- Settings Mode
- Voice Parameter Mode
- Parameter Length Mode

### 6. Parameter System
- Core Parameters
- AS5600 Control
- Distance Sensor Recording
- Parameter Lengths

### 7. Voice System
- Voice 1 vs Voice 2
- Voice Presets
- Voice Configuration
- Real-time Voice Parameters

### 8. Advanced Features
- Scales and Tuning
- LED Themes
- Shuffle/Swing Patterns
- Polyrhythmic Sequences

### 9. LED Feedback System
- Color Coding
- Mode Indicators
- Playhead Display
- Parameter Visualization

### 10. Troubleshooting
- Common Issues
- Reset Procedures
- Calibration

---

## Detailed Content Plan

### Quick Start Guide Content

#### Button Layout (32 buttons total)
```
Step Buttons (0-15): 4x4 grid for sequence steps
┌─────┬─────┬─────┬─────┐
│  0  │  1  │  2  │  3  │
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │  7  │
├─────┼─────┼─────┼─────┤
│  8  │  9  │ 10  │ 11  │
├─────┼─────┼─────┼─────┤
│ 12  │ 13  │ 14  │ 15  │
└─────┴─────┴─────┴─────┘

Parameter Buttons (16-21):
16: Note     17: Velocity  18: Filter
19: Attack   20: Decay     21: Octave

Mode/Control Buttons (22-31):
22: Slide Mode        23: Delay Toggle
24: Voice Switch      25: AS5600 Control
26: Play/Stop         27: Scale Change
28: Theme Change      29: Swing Pattern
30: Randomize Seq1    31: Randomize Seq2
```

#### Mode Quick Reference

**Normal Mode:**
- Short press step: Toggle gate on/off
- Long press step: Select for editing
- Hold parameter + move hand: Record values
- AS5600: Control selected parameter

**Slide Mode (Button 22):**
- Step buttons: Toggle slide on/off
- LED shows slide states
- Exit: Press button 22 again

**Settings Mode (Long press Play/Stop when stopped):**
- Main menu: Steps 0-1 select Voice 1/2
- Preset selection: Steps 0-5 select presets
- Voice parameters: Buttons 9-24 toggle features
- Exit: Start playback

**Voice Parameter Mode:**
- Buttons 9-13: Toggle voice features
- Button 9: Envelope on/off
- Button 10: Overdrive on/off
- Button 11: Wavefolder on/off
- Button 12: Filter mode cycle
- Button 13: Filter resonance

#### Essential Controls
- **Play/Stop:** Button 26
- **Voice Switch:** Button 24 (Voice 1 ↔ Voice 2)
- **Parameter Control:** AS5600 encoder
- **Recording:** Hold parameter button + distance sensor
- **Settings:** Long press Play/Stop when stopped

### Hardware Overview Content

#### LED Matrix (8x4 = 32 LEDs)
- **Rows 0-1:** Voice 1 steps (0-15)
- **Rows 2-3:** Voice 2 steps (0-15)
- **Color coding:** Different colors for each voice
- **Breathing effect:** When idle
- **Playhead:** Bright accent color

#### Control LEDs
- Voice 1/2 indicators
- Delay time/feedback
- Mode status indicators

#### AS5600 Rotary Encoder
- **Velocity-sensitive:** Faster rotation = bigger changes
- **Parameter modes:** 8 different parameters
- **Bidirectional:** Clockwise/counterclockwise
- **Button:** Cycle through parameter modes

#### Distance Sensor
- **Real-time recording:** Hold parameter button
- **Range:** Hand position controls parameter value
- **Visual feedback:** OLED shows current value

### Button Reference Content

#### Step Buttons (0-15)
- **Short Press:** Toggle step gate on/off
- **Long Press:** Select step for editing
- **With Parameter Button:** Set parameter length
- **In Slide Mode:** Toggle slide on/off
- **In Settings:** Navigate menus

#### Parameter Buttons (16-21)
- **Button 16 - Note:** Note selection (0-21 scale degrees)
- **Button 17 - Velocity:** Note velocity (0.0-1.0)
- **Button 18 - Filter:** Filter cutoff (0.0-1.0)
- **Button 19 - Attack:** Envelope attack (0.0-1.0s)
- **Button 20 - Decay:** Envelope decay (0.0-1.0s)
- **Button 21 - Octave:** Octave offset (-1, 0, +1)

**Usage:**
- **Hold + Step:** Set parameter length
- **Hold + Distance Sensor:** Record parameter values
- **Press in Edit Mode:** Toggle parameter editing

#### Mode/Control Buttons (22-31)

**Button 22 - Slide Mode:**
- **Press:** Enter/exit slide mode
- **In Slide Mode:** Step buttons toggle slide

**Button 23 - Delay Toggle:**
- **Press:** Toggle delay effect on/off

**Button 24 - Voice Switch:**
- **Press:** Switch between Voice 1 and Voice 2
- **Long Press:** Reserved for future LFO mode

**Button 25 - AS5600 Control:**
- **Press:** Cycle through AS5600 parameter modes
- **Double Press:** Reset to default parameter

**Button 26 - Play/Stop:**
- **Short Press:** Start/stop sequencer
- **Long Press (when stopped):** Enter settings mode

**Button 27 - Scale Change:**
- **Press:** Cycle through musical scales
- **Scales:** Mixolydian, Pentatonic Minor, Dorian, Phrygian Dominant, Lydian Dominant, Wholetone, Chromatic

**Button 28 - Theme Change:**
- **Press:** Cycle through LED color themes
- **5 Themes:** Different color palettes for LED matrix

**Button 29 - Swing Pattern:**
- **Press:** Cycle through shuffle/swing templates
- **Affects:** Timing feel of sequences

**Button 30 - Randomize Seq1:**
- **Short Press:** Randomize Voice 1 parameters
- **Long Press:** Reset Voice 1 to defaults

**Button 31 - Randomize Seq2:**
- **Short Press:** Randomize Voice 2 parameters
- **Long Press:** Reset Voice 2 to defaults

### Operating Modes Content

#### Normal Sequencing Mode
- **Default mode** when sequencer is running
- Step buttons control gate on/off
- Parameter buttons enable real-time control
- LED matrix shows gate states and playhead
- OLED displays current step and parameter values

**Workflow:**
1. Press step buttons to create gate pattern
2. Hold parameter button + move hand over sensor
3. Watch OLED for real-time parameter values
4. Use AS5600 for fine adjustments
5. Switch voices with button 24

#### Slide Mode (Button 22)
- **Purpose:** Add legato/slide between notes
- **Entry:** Press button 22
- **Visual:** LED matrix shows slide states
- **Controls:** Step buttons toggle slide on/off
- **Exit:** Press button 22 again

**Slide Behavior:**
- Slide active: Note glides to next note
- Slide inactive: Note triggers normally
- Per-step control: Each step can have slide
- Voice-specific: Voice 1 and Voice 2 independent

#### Settings Mode
- **Entry:** Long press Play/Stop when stopped
- **Exit:** Start playback or press Play/Stop
- **Two levels:** Main menu → Preset selection

**Main Menu:**
- Step 0: Voice 1 configuration
- Step 1: Voice 2 configuration
- LED shows current selection with pulsing

**Preset Selection:**
- Steps 0-5: Six voice presets
- Current preset highlighted
- Apply immediately when selected

**Voice Presets:**
0. **Analog:** Classic analog synth (3 detuned saws)
1. **Digital:** Digital/FM-style sounds
2. **Bass:** Low-end focused configuration
3. **Lead:** Lead synth configuration
4. **Pad:** Ambient pad sounds
5. **Percussion:** Percussive/noise elements

#### Voice Parameter Mode
- **Entry:** Press buttons 9-13 in settings mode
- **Duration:** 3 seconds after last change
- **Visual:** LED shows affected parameter
- **OLED:** Displays parameter status

**Voice Parameters:**
- **Button 9:** Envelope on/off
- **Button 10:** Overdrive on/off
- **Button 11:** Wavefolder on/off
- **Button 12:** Filter mode (LP12/LP24/LP36/BP12/BP24)
- **Button 13:** Filter resonance (0.0-1.0)
- **Button 14:** Dalek effect on/off

#### Parameter Length Mode
- **Entry:** Hold any parameter button
- **Purpose:** Set different sequence lengths per parameter
- **Visual:** LED shows parameter length and playhead
- **Range:** 1-16 steps per parameter

**Usage:**
1. Hold parameter button (16-21)
2. Press step button for desired length
3. LED shows active steps for that parameter
4. Release parameter button to exit

**Polyrhythmic Sequences:**
- Different parameters can have different lengths
- Creates complex rhythmic patterns
- Each parameter loops independently
- Visual overlay shows multiple playheads

### Parameter System Content

#### Core Parameters

**Note (Button 16):**
- **Range:** 0-21 (scale degrees)
- **Function:** Selects note from current scale
- **Recording:** Distance sensor maps to scale degrees
- **AS5600:** Fine adjustment of note selection

**Velocity (Button 17):**
- **Range:** 0.0-1.0
- **Function:** Note volume/intensity
- **Recording:** Hand distance = velocity level
- **AS5600:** Precise velocity control

**Filter (Button 18):**
- **Range:** 0.0-1.0
- **Function:** Filter cutoff frequency
- **Recording:** Real-time filter sweeps
- **AS5600:** Filter modulation

**Attack (Button 19):**
- **Range:** 0.0-1.0 seconds
- **Function:** Envelope attack time
- **Recording:** Dynamic attack per step
- **AS5600:** Attack time adjustment

**Decay (Button 20):**
- **Range:** 0.0-1.0 seconds
- **Function:** Envelope decay time
- **Recording:** Decay time per step
- **AS5600:** Decay time adjustment

**Octave (Button 21):**
- **Range:** -1, 0, +1 octaves
- **Function:** Octave transposition
- **Recording:** Hand position selects octave
- **AS5600:** Octave switching

#### AS5600 Parameter Modes

**Mode Cycling:** Press button 25 to cycle through:

1. **Velocity Mode:** Control note velocity
2. **Filter Mode:** Control filter cutoff
3. **Attack Mode:** Control envelope attack
4. **Decay Mode:** Control envelope decay
5. **Note Mode:** Control note selection
6. **Delay Time Mode:** Control global delay time
7. **Delay Feedback Mode:** Control delay feedback
8. **Slide Time Mode:** Control slide/glide time

**Velocity-Sensitive Control:**
- **Slow rotation:** Fine adjustments
- **Fast rotation:** Coarse adjustments
- **Bidirectional:** Clockwise/counterclockwise
- **Visual feedback:** Control LEDs show current mode

#### Distance Sensor Recording

**Real-Time Recording:**
1. Hold parameter button (16-21)
2. Move hand over distance sensor
3. Watch OLED for current value
4. Parameter recorded to current step
5. Release button to stop recording

**Recording Behavior:**
- **Immediate feedback:** Audio changes in real-time
- **Step-based:** Records to currently playing step
- **Visual confirmation:** OLED shows parameter name and value
- **Range mapping:** Hand distance maps to parameter range

### Voice System Content

#### Voice Architecture
- **Two Independent Voices:** Voice 1 and Voice 2
- **Separate Sequences:** Each voice has its own sequencer
- **Individual Parameters:** All parameters per-voice
- **Visual Separation:** LED matrix rows 0-1 (Voice 1), rows 2-3 (Voice 2)

#### Voice Switching (Button 24)
- **Current Voice:** Determines which sequencer is active
- **Parameter Control:** AS5600 and distance sensor affect current voice
- **Visual Indicator:** Control LEDs show active voice
- **OLED Display:** Shows current voice in header

#### Voice Presets

**Preset 0 - Analog:**
- 3 detuned sawtooth oscillators
- Classic analog filter
- Envelope and overdrive enabled
- Warm, vintage character

**Preset 1 - Digital:**
- Digital waveforms
- Sharper filter characteristics
- Modern, precise sound
- Good for leads and arpeggios

**Preset 2 - Bass:**
- Low-frequency optimized
- Strong fundamental
- Punchy envelope
- Ideal for basslines

**Preset 3 - Lead:**
- Cutting lead tones
- Enhanced harmonics
- Expressive filter
- Solo-ready sounds

**Preset 4 - Pad:**
- Ambient textures
- Soft attack
- Long decay
- Atmospheric sounds

**Preset 5 - Percussion:**
- Noise elements
- Sharp attacks
- Short decays
- Rhythmic textures

#### Real-Time Voice Parameters

**Envelope (Button 9):**
- **On:** Full ADSR envelope
- **Off:** Direct gate control
- **Affects:** Note articulation

**Overdrive (Button 10):**
- **On:** Analog-style saturation
- **Off:** Clean signal
- **Affects:** Harmonic content

**Wavefolder (Button 11):**
- **On:** Wave shaping distortion
- **Off:** Clean waveform
- **Affects:** Timbral complexity

**Filter Mode (Button 12):**
- **LP12:** 12dB lowpass
- **LP24:** 24dB lowpass
- **LP36:** 36dB lowpass
- **BP12:** 12dB bandpass
- **BP24:** 24dB bandpass

**Filter Resonance (Button 13):**
- **Range:** 0.0-1.0
- **Effect:** Filter emphasis
- **High values:** Self-oscillation

**Dalek Effect (Button 14):**
- **On:** Ring modulation effect
- **Off:** Normal voice
- **Affects:** Metallic, robotic timbre

### Advanced Features Content

#### Musical Scales (Button 27)

**Scale Selection:** Cycles through 7 scales:

1. **Mixolydian:** Major scale with flat 7th
2. **Pentatonic Minor:** 5-note minor scale
3. **Dorian:** Minor scale with raised 6th
4. **Phrygian Dominant:** Exotic scale with flat 2nd
5. **Lydian Dominant:** Major scale with sharp 4th and flat 7th
6. **Wholetone:** All whole steps
7. **Chromatic:** All 12 semitones

**Note Parameter Mapping:**
- Note parameter (0-21) maps to scale degrees
- Automatic transposition to selected scale
- Real-time scale changes affect all voices
- OLED displays current scale name

#### LED Themes (Button 28)

**Theme Selection:** 5 color palettes:

1. **Default:** Green/orange classic
2. **Ocean:** Blue/cyan aquatic
3. **Forest:** Green/yellow natural
4. **Sunset:** Purple/magenta warm
5. **Arctic:** Cyan/blue cool

**Theme Elements:**
- Gate on/off colors per voice
- Playhead accent color
- Parameter mode colors
- Breathing effect color
- Edit mode indicators

#### Shuffle/Swing Patterns (Button 29)

**Timing Templates:**
- Multiple shuffle patterns available
- Affects timing feel of sequences
- Applied globally to both voices
- Real-time pattern switching

**Pattern Types:**
- Straight timing
- Light swing
- Heavy swing
- Triplet feel
- Custom patterns

#### Polyrhythmic Sequences

**Independent Parameter Lengths:**
- Each parameter can have 1-16 steps
- Creates complex rhythmic relationships
- Visual overlay shows multiple playheads
- Mathematical relationships create patterns

**Example Polyrhythms:**
- Gate: 16 steps, Note: 12 steps = 4:3 polyrhythm
- Velocity: 8 steps, Filter: 6 steps = 4:3 polyrhythm
- Attack: 4 steps, Decay: 3 steps = 4:3 polyrhythm

### LED Feedback System Content

#### Color Coding System

**Voice 1 Colors:**
- **Gate On:** Bright green
- **Gate Off:** Dim green
- **Playhead:** Orange accent
- **Edit Mode:** Blue tint

**Voice 2 Colors:**
- **Gate On:** Bright cyan
- **Gate Off:** Dim cyan
- **Playhead:** Orange accent
- **Edit Mode:** Blue tint

**Parameter Colors:**
- **Note:** Blue spectrum
- **Velocity:** Green spectrum
- **Filter:** Purple spectrum
- **Attack:** Red spectrum
- **Decay:** Yellow spectrum
- **Octave:** White spectrum

#### Mode Indicators

**Normal Mode:**
- Gate states clearly visible
- Playhead moves with sequence
- Smooth color transitions

**Slide Mode:**
- Slide states overlay gate colors
- Active slides highlighted
- Clear on/off indication

**Settings Mode:**
- Menu options highlighted
- Current selection pulsing
- Clear navigation feedback

**Parameter Length Mode:**
- Active steps highlighted
- Parameter playhead visible
- Length clearly indicated

**Voice Parameter Mode:**
- Affected parameter highlighted
- 3-second timeout indication
- Clear parameter identification

#### Breathing Effect
- **When:** Both sequencers stopped, no step selected
- **Color:** Soft blue
- **Pattern:** Slow sine wave
- **Purpose:** Indicates idle state

### Troubleshooting Content

#### Common Issues

**No Sound:**
1. Check audio connections
2. Verify voice is not muted
3. Check gate pattern (steps enabled)
4. Verify velocity levels
5. Check filter cutoff (not fully closed)

**Erratic Parameter Control:**
1. Calibrate AS5600 encoder
2. Check distance sensor positioning
3. Verify hand movement range
4. Reset parameter base values

**LED Issues:**
1. Check power supply
2. Verify LED matrix connections
3. Try different theme
4. Reset to defaults

**Timing Issues:**
1. Check shuffle pattern setting
2. Verify parameter lengths
3. Reset sequence lengths
4. Check clock source

#### Reset Procedures

**Parameter Reset:**
- Long press randomize buttons (30/31)
- Resets all parameters to defaults
- Clears gate patterns
- Resets parameter lengths

**Voice Reset:**
- Enter settings mode
- Reapply voice preset
- Toggles voice parameters off/on
- Resets voice configuration

**System Reset:**
- Power cycle device
- All settings return to defaults
- Sequences cleared
- Calibration reset

#### Calibration

**AS5600 Encoder:**
1. Center encoder position
2. Press button 25 twice quickly
3. Verify smooth parameter control
4. Test velocity sensitivity

**Distance Sensor:**
1. Position hand at comfortable distance
2. Record parameter values
3. Verify full range coverage
4. Adjust hand position as needed

**LED Brightness:**
- Automatic brightness adjustment
- No manual calibration needed
- Check power supply if dim

---

## Implementation Notes

### Documentation Structure
- **Modular sections:** Each section can be read independently
- **Cross-references:** Links between related sections
- **Visual aids:** Diagrams and button layouts
- **Examples:** Practical usage scenarios

### Visual Elements
- **Button layout diagrams:** ASCII art for clarity
- **LED color charts:** Visual color coding reference
- **Workflow diagrams:** Step-by-step processes
- **Parameter range tables:** Quick reference values

### User Experience
- **Progressive complexity:** Basic to advanced features
- **Quick reference:** Essential information easily accessible
- **Troubleshooting:** Common issues and solutions
- **Examples:** Real-world usage scenarios

This comprehensive manual plan covers all aspects of the PicoMudrasSequencer, from basic operation to advanced features, ensuring users can fully utilize the instrument's capabilities.