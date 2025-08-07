# PicoMudrasSequencer Quick Start Guide

## Button Layout (32 Buttons Total)

### Step Buttons (0-31) - 4x8 LED Matrix Grid
```
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │  Row 0
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│  8  │  9  │ 10  │ 11  │ 12  │ 13  │ 14  │ 15  │  Row 1
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 16  │ 17  │ 18  │ 19  │ 20  │ 21  │ 22  │ 23  │  Row 2
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 24  │ 25  │ 26  │ 27  │ 28  │ 29  │ 30  │ 31  │  Row 3
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘

• Short Press: Toggle gate on/off
• Long Press: Select for editing  
• With Param Button: Set length
• In Slide Mode: Toggle slide
• In Settings: Navigate menus
```

### Parameter Buttons (16-22)
| Button | Parameter | Range | Function |
|--------|-----------|-------|----------|
| **16** | Note | 0-21 | Scale degree selection |
| **17** | Velocity | 0.0-1.0 | Note volume/intensity |
| **18** | Filter | 0.0-1.0 | Filter cutoff frequency |
| **19** | Attack | 0.0-1.0s | Envelope attack time |
| **20** | Decay | 0.0-1.0s | Envelope decay time |
| **21** | Octave | -1,0,+1 | Octave transposition |
| **22** | Slide | 0/1 | Slide/legato enable |

**Usage:** Hold + Distance Sensor = Record values • Hold + Step = Set length

### Control Buttons (23-31)
| Button | Function | Short Press | Long Press |
|--------|----------|-------------|------------|
| **23** | Delay | Toggle delay on/off | - |
| **24** | Voice Switch | Voice 1 ↔ Voice 2 | (Reserved) |
| **25** | AS5600 Control | Cycle parameter modes | - |
| **26** | Play/Stop | Start/stop sequencer | Settings mode |
| **27** | Scale | Cycle musical scales | - |
| **28** | Theme | Cycle LED themes | - |
| **29** | Swing | Cycle shuffle patterns | - |
| **30** | Randomize 1 | Randomize Voice 1 | Reset Voice 1 |
| **31** | Randomize 2 | Randomize Voice 2 | Reset Voice 2 |

## Operating Modes

### 🎵 Normal Mode (Default)
- **Step buttons:** Toggle gates, long press to edit
- **Parameter buttons:** Hold + distance sensor to record
- **AS5600:** Control selected parameter
- **LED:** Shows gate states and playhead

### 🎼 Slide Mode (Parameter Button 22)
- **Entry:** Hold parameter button 22 (Slide)
- **Step buttons:** Toggle slide on/off per step
- **LED:** Shows slide states overlaid on gates
- **Exit:** Release parameter button 22

### ⚙️ Settings Mode (Long press Play/Stop when stopped)
**Main Menu:**
- **Step 0:** Voice 1 configuration
- **Step 1:** Voice 2 configuration

**Preset Selection:** (After selecting voice)
- **Steps 0-5:** Voice presets (Analog, Digital, Bass, Lead, Pad, Percussion)

**Voice Parameters:** (Buttons 9-14)
- **Button 9:** Envelope on/off
- **Button 10:** Overdrive on/off  
- **Button 11:** Wavefolder on/off
- **Button 12:** Filter mode (LP12/LP24/LP36/BP12/BP24)
- **Button 13:** Filter resonance (0.0-1.0)
- **Button 14:** Dalek effect on/off

**Exit:** Start playback

### 📏 Parameter Length Mode
- **Entry:** Hold any parameter button (16-21)
- **Step buttons:** Set sequence length (1-16 steps)
- **LED:** Shows active steps for parameter
- **Exit:** Release parameter button

## AS5600 Encoder Control (Button 25)

**Parameter Modes:** (Press button 25 to cycle)
1. **Velocity** - Note volume control
2. **Filter** - Filter cutoff control  
3. **Attack** - Envelope attack control
4. **Decay** - Envelope decay control
5. **Note** - Note selection control
6. **Delay Time** - Global delay time
7. **Delay Feedback** - Delay feedback amount
8. **Slide Time** - Slide/glide duration

**Control:** Velocity-sensitive (fast = coarse, slow = fine)

## Musical Scales (Button 27)
1. **Mixolydian** - Major with ♭7
2. **Pentatonic Minor** - 5-note minor
3. **Dorian** - Minor with ♯6
4. **Phrygian Dominant** - Exotic with ♭2
5. **Lydian Dominant** - Major with ♯4, ♭7
6. **Wholetone** - All whole steps
7. **Chromatic** - All 12 semitones

## LED Color Guide

### Voice Colors
- **Voice 1:** Green (bright=on, dim=off)
- **Voice 2:** Cyan (bright=on, dim=off)
- **Playhead:** Orange accent
- **Slide Active:** Blended highlight
- **Idle:** Blue breathing effect

### Mode Indicators
- **Settings:** Pulsing selection colors
- **Parameter Length:** Parameter-specific colors
- **Voice Parameters:** Feature-specific colors
- **Edit Mode:** Blue tint overlay

## Essential Workflows

### 🚀 Basic Sequence Creation
1. Press **Play/Stop (26)** to start
2. Press **step buttons (0-31)** to create gate pattern
3. Hold **parameter button (16-22)** + move hand over sensor
4. Use **AS5600** for fine adjustments
5. Switch voices with **Voice Switch (24)**

### 🎛️ Parameter Recording
1. Hold **parameter button** (Note, Velocity, Filter, etc.)
2. Move hand over **distance sensor**
3. Watch **OLED** for real-time values
4. Release button when satisfied
5. Parameter recorded to current step

### 🔄 Voice Configuration
1. **Long press Play/Stop** when stopped
2. Select **Voice 1 (step 0)** or **Voice 2 (step 1)**
3. Choose **preset (steps 0-5)**
4. Adjust **voice parameters (buttons 9-14)**
5. **Start playback** to exit

### 🎵 Slide/Legato Setup
1. Hold **Slide parameter button (22)**
2. Press **step buttons** to toggle slide per step
3. LED shows slide states
4. Release **Slide parameter button (22)** to exit

### 📐 Polyrhythmic Sequences
1. Hold **parameter button** (e.g., Note)
2. Press **step button** for desired length (e.g., step 11 = 12 steps)
3. Repeat for other parameters with different lengths
4. Creates complex polyrhythmic patterns

## Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| No sound | Check gates enabled, velocity > 0, filter not closed |
| Erratic control | Recalibrate AS5600 (double-press button 25) |
| Wrong voice | Check voice indicator LEDs, press Voice Switch (24) |
| Timing issues | Check shuffle pattern (button 29), parameter lengths |
| LED problems | Try different theme (button 28), check power |

## Reset Commands
- **Parameter Reset:** Long press Randomize buttons (30/31)
- **Voice Reset:** Reapply preset in settings mode
- **Full Reset:** Power cycle device

---

**💡 Pro Tips:**
- Use different parameter lengths for polyrhythmic complexity
- Combine slide mode with filter sweeps for expressive sequences
- Layer Voice 1 and Voice 2 for rich harmonic content
- Experiment with different scales for various musical moods
- Use velocity-sensitive AS5600 control for precise adjustments

**🎯 Remember:** With 4x8 matrix, you have 32 buttons total for compact yet powerful sequencing