#include <Arduino.h>
#include "LEDMatrixFeedback.h"
#include <FastLED.h>
#include <cmath>

#include "ledMatrix.h"
#include "../sequencer/Sequencer.h"
#include "../ui/UIEventHandler.h"
#include "../ui/ButtonManager.h"
#include "../utils/Debug.h"  // Add debug support

int ledOffset = 24;

CRGB smoothedTargetColors[LEDMatrix::WIDTH * LEDMatrix::HEIGHT];

const uint8_t TARGET_SMOOTHING_BLEND_AMOUNT = 180;

// Define common colors as constants for readability and maintainability
// These will be populated from the activeThemeColors pointer
CRGB current_COLOR_GATE_ON_V1;
CRGB current_COLOR_GATE_OFF_V1;
CRGB current_COLOR_PLAYHEAD_ACCENT;
CRGB current_COLOR_GATE_ON_V2;
CRGB current_COLOR_GATE_OFF_V2;
CRGB current_COLOR_IDLE_BREATHING_BLUE;
CRGB current_COLOR_EDIT_MODE_DIM_BLUE_V1;
CRGB current_COLOR_EDIT_MODE_DIM_BLUE_V2;
CRGB current_COLOR_MOD_NOTE_ACTIVE;
CRGB current_COLOR_MOD_NOTE_INACTIVE;
CRGB current_COLOR_MOD_VELOCITY_ACTIVE;
CRGB current_COLOR_MOD_VELOCITY_INACTIVE;
CRGB current_COLOR_MOD_FILTER_ACTIVE;
CRGB current_COLOR_MOD_FILTER_INACTIVE;
CRGB current_COLOR_MOD_DECAY_ACTIVE;
CRGB current_COLOR_MOD_DECAY_INACTIVE;
CRGB current_COLOR_MOD_OCTAVE_ACTIVE;
CRGB current_COLOR_MOD_OCTAVE_INACTIVE;
CRGB current_COLOR_DEFAULT_ACTIVE;
CRGB current_COLOR_DEFAULT_INACTIVE;
CRGB current_COLOR_MOD_PARAM_MODE_ACTIVE;
CRGB current_COLOR_MOD_PARAM_MODE_INACTIVE;
CRGB current_COLOR_MOD_GATE_MODE_ACTIVE;
CRGB current_COLOR_MOD_GATE_MODE_INACTIVE;
CRGB current_COLOR_RANDOMIZE_FLASH;
CRGB current_COLOR_RANDOMIZE_IDLE;

const LEDThemeColors ALL_THEMES[] = {
    {
        CRGB(0, 188, 0), CRGB(5, 22, 5), CRGB(188, 94, 0), CRGB(0, 128, 128), CRGB(0, 8, 8),
        CRGB(0, 0, 94), CRGB(0, 0, 12), CRGB(0, 0, 12), CRGB(128, 94, 0), CRGB(32, 24, 0),
        CRGB(94, 0, 94), CRGB(24, 0, 24), CRGB(0, 94, 188), CRGB(0, 24, 48), CRGB(188, 64, 0),
        CRGB(48, 16, 0), CRGB(128, 0, 0), CRGB(32, 0, 0), CRGB(0, 128, 64), CRGB(0, 32, 16),
        CRGB(188, 0, 188), CRGB(48, 0, 48), CRGB(64, 64, 128), CRGB(16, 16, 32), CRGB(128, 64, 0),
        CRGB(32, 16, 0), CRGB(94, 0, 64), CRGB(24, 0, 16), CRGB(64, 94, 94), CRGB(16, 24, 24)
    },
    {
        CRGB(0, 120, 188), CRGB(0, 12, 24), CRGB(64, 156, 188), CRGB(0, 144, 166), CRGB(0, 18, 12),
        CRGB(0, 48, 144), CRGB(0, 5, 25), CRGB(0, 12, 17), CRGB(0, 144, 188), CRGB(0, 15, 22),
        CRGB(64, 144, 188), CRGB(13, 29, 38), CRGB(94, 0, 188), CRGB(11, 0, 24), CRGB(188, 144, 0),
        CRGB(38, 29, 0), CRGB(144, 188, 94), CRGB(29, 38, 19), CRGB(188, 0, 94), CRGB(17, 0, 11),
        CRGB(144, 0, 188), CRGB(29, 0, 38), CRGB(48, 144, 144), CRGB(10, 29, 29), CRGB(0, 166, 188),
        CRGB(0, 33, 38), CRGB(144, 0, 188), CRGB(15, 0, 22), CRGB(0, 188, 166), CRGB(0, 22, 15)
    },
    {
        CRGB(0, 188, 94), CRGB(0, 24, 8), CRGB(94, 188, 0), CRGB(0, 144, 72), CRGB(0, 15, 6),
        CRGB(0, 94, 47), CRGB(0, 11, 4), CRGB(0, 16, 5), CRGB(188, 144, 0), CRGB(18, 13, 0),
        CRGB(47, 188, 0), CRGB(5, 19, 0), CRGB(0, 188, 144), CRGB(0, 24, 14), CRGB(188, 94, 0),
        CRGB(24, 8, 0), CRGB(144, 188, 47), CRGB(12, 17, 5), CRGB(144, 0, 94), CRGB(15, 0, 11),
        CRGB(188, 47, 144), CRGB(18, 5, 14), CRGB(94, 144, 47), CRGB(7, 14, 5), CRGB(166, 188, 0),
        CRGB(14, 15, 0), CRGB(0, 188, 94), CRGB(0, 22, 8), CRGB(144, 188, 144), CRGB(14, 22, 14)
    },
    {
        CRGB(0, 188, 188), CRGB(0, 38, 38), CRGB(188, 0, 188), CRGB(0, 144, 188), CRGB(0, 29, 38),
        CRGB(94, 0, 188), CRGB(47, 0, 94), CRGB(71, 0, 144), CRGB(188, 0, 94), CRGB(38, 0, 19),
        CRGB(0, 188, 94), CRGB(0, 38, 19), CRGB(94, 144, 188), CRGB(19, 29, 38), CRGB(188, 188, 0),
        CRGB(38, 38, 0), CRGB(188, 94, 0), CRGB(38, 19, 0), CRGB(188, 0, 144), CRGB(38, 0, 29),
        CRGB(0, 188, 188), CRGB(0, 38, 38), CRGB(144, 94, 188), CRGB(29, 19, 38), CRGB(188, 47, 0),
        CRGB(38, 10, 0), CRGB(47, 188, 188), CRGB(10, 38, 38), CRGB(188, 188, 188), CRGB(38, 38, 38)
    },
    {
        CRGB(0, 188, 144), CRGB(0, 12, 15), CRGB(144, 188, 188), CRGB(94, 144, 188), CRGB(5, 12, 22),
        CRGB(47, 94, 188), CRGB(12, 12, 55), CRGB(36, 71, 144), CRGB(144, 0, 188), CRGB(29, 0, 38),
        CRGB(0, 144, 188), CRGB(0, 29, 38), CRGB(47, 188, 144), CRGB(10, 38, 29), CRGB(188, 144, 188),
        CRGB(38, 29, 38), CRGB(188, 94, 144), CRGB(38, 19, 29), CRGB(94, 0, 188), CRGB(19, 0, 38),
        CRGB(188, 144, 0), CRGB(38, 29, 0), CRGB(94, 144, 166), CRGB(19, 29, 33), CRGB(166, 144, 188),
        CRGB(33, 29, 38), CRGB(144, 188, 166), CRGB(29, 38, 33), CRGB(188, 188, 166), CRGB(38, 38, 33)
    }
};

static const LEDThemeColors* activeThemeColors = &ALL_THEMES[static_cast<int>(LEDTheme::DEFAULT)];

void setLEDTheme(LEDTheme theme) {
    if (static_cast<int>(theme) < static_cast<int>(LEDTheme::COUNT)) {
        activeThemeColors = &ALL_THEMES[static_cast<int>(theme)];
    }
}

const LEDThemeColors* getActiveThemeColors() {
    return activeThemeColors;
}

DEFINE_GRADIENT_PALETTE( parameterPalette ) {
    0,   0,   0,  255,   // Blue
    85,  0,   255, 44,    // Green
    170, 200, 66,   0,    // Red
    255, 0,   66,  255    // Back to blue
};
CRGBPalette16 parameterColors = parameterPalette;

CRGB getParameterColor(ParamId param, uint8_t intensity) {
    uint8_t paletteIndex = map(static_cast<int>(param), 0, static_cast<int>(ParamId::Count), 0, 255);
    return ColorFromPalette(parameterColors, paletteIndex, intensity);
}

void addPolyrhythmicOverlay(
    LEDMatrix& ledMatrix,
    const Sequencer& seq,
    bool secondInPair,
    uint8_t intensity = 32
) {
    if (!seq.isRunning()) return;

    const int baseOffset = secondInPair ? 32 : 0; // Fixed offset for 8x8 matrix
    constexpr size_t NUM_PARAMS = 3;

    // Define parameter/color pairs for overlay
    struct OverlayParam {
        ParamId param;
        CRGB color;
    };

    const OverlayParam overlayParams[NUM_PARAMS] = {
        {ParamId::Note,    CRGB(0, intensity, intensity)},
        {ParamId::Velocity,CRGB(0, intensity, 0)},
        {ParamId::Filter,  CRGB(0, 0, intensity)}
    };

    for (size_t i = 0; i < NUM_PARAMS; ++i) {
        const ParamId param = overlayParams[i].param;
        const uint8_t paramStep = seq.getCurrentStepForParameter(param);
        const uint8_t paramLength = seq.getParameterStepCount(param);

        // Only overlay if within LED matrix bounds and valid step count
        if (paramStep < 16 && paramLength > 1 && paramLength <= 16) {
            const int ledIndex = baseOffset + paramStep;
            CRGB blendedColor = ledMatrix.getLeds()[ledIndex];
            blendedColor += overlayParams[i].color;

            // Calculate LED coordinates from paramStep
            const int x = paramStep % LEDMatrix::WIDTH;
            // Row index adjusted for secondInPair (bottom half starts at row 4)
            const int y = paramStep / LEDMatrix::WIDTH + (secondInPair ? 4 : 0);

            ledMatrix.setLED(x, y, blendedColor);
        }
    }
}

float ease(float x) {
    return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
}

float smoothBreathing(uint32_t ms) {
    float t = (float)(ms % 2000) / 2000.0f;
    return ease(0.5f * (1.0f + sin(2.0f * PI * t)));
}

void setStepLedColor(LEDMatrix& ledMatrix, uint8_t step, uint8_t r, uint8_t g, uint8_t b) {
    int x = step % LEDMatrix::WIDTH;
    int y = step / LEDMatrix::WIDTH;
    ledMatrix.setLED(x, y, CRGB(r, g, b));
}

void setupLEDMatrixFeedback() {
    for (int i = 0; i < LEDMatrix::WIDTH * LEDMatrix::HEIGHT; ++i) {
        smoothedTargetColors[i] = CRGB::Black;
    }
}

/**
 * @brief Updates LED matrix to show settings mode interface
 *
 * Displays menu options and preset selections using step LEDs:
 * - Main menu: Shows Voice 1 and Voice 2 options (steps 0-1)
 * - Preset selection: Shows available presets (steps 0-5 for 6 presets)
 * - Uses different colors to indicate current selection and available options
 */
void updateSettingsModeLEDs(LEDMatrix& ledMatrix, const UIState& uiState) {
    const LEDThemeColors* activeThemeColors = getActiveThemeColors();

    // Clear all LEDs first
    for (int i = 0; i < LEDMatrix::WIDTH * LEDMatrix::HEIGHT; ++i) {
        ledMatrix.getLeds()[i] = CRGB::Black;
    }

    if (uiState.inPresetSelection) {
        // Preset selection mode - show available presets
        const uint8_t presetCount = 6; // VoicePresets::getPresetCount() returns 6

        // Define preset colors based on voice being configured
        CRGB selectedColor = (uiState.settingsMenuIndex == 0) ?
            activeThemeColors->gateOnV1 : activeThemeColors->gateOnV2;
        CRGB availableColor = (uiState.settingsMenuIndex == 0) ?
            activeThemeColors->gateOffV1 : activeThemeColors->gateOffV2;

        // Show presets in first 6 step positions (0-5)
        for (uint8_t i = 0; i < presetCount && i < 16; i++) {
            CRGB color;

            // Highlight currently selected preset
            uint8_t currentPresetIndex = (uiState.settingsMenuIndex == 0) ?
                uiState.voice1PresetIndex : uiState.voice2PresetIndex;

            if (i == currentPresetIndex) {
                // Current preset - bright pulsing
                uint32_t time = millis();
                float pulse = 0.5f + 0.5f * sinf(time * 0.008f);
                color = selectedColor;
                color.nscale8(static_cast<uint8_t>(128 + 127 * pulse));
            } else {
                // Available preset - dim steady
                color = availableColor;
                color.nscale8(64);
            }

            // Calculate LED position
            int x = i % LEDMatrix::WIDTH;
            int y = i / LEDMatrix::WIDTH;
            ledMatrix.setLED(x, y, color);
        }

        // Show which voice is being configured in bottom row
        if (uiState.settingsMenuIndex == 0) {
            // Voice 1 indicator
            ledMatrix.setLED(0, 7, activeThemeColors->gateOnV1);
        } else {
            // Voice 2 indicator
            ledMatrix.setLED(1, 7, activeThemeColors->gateOnV2);
        }

    } else {
        // Main settings menu - show Voice 1 and Voice 2 options

        // Voice 1 option (step 0)
        CRGB voice1Color = (uiState.settingsMenuIndex == 0) ?
            activeThemeColors->gateOnV1 : activeThemeColors->gateOffV1;
        if (uiState.settingsMenuIndex == 0) {
            // Add pulsing effect for selected option
            uint32_t time = millis();
            float pulse = 0.5f + 0.5f * sinf(time * 0.006f);
            voice1Color.nscale8(static_cast<uint8_t>(128 + 127 * pulse));
        } else {
            voice1Color.nscale8(96);
        }
        ledMatrix.setLED(0, 0, voice1Color);

        // Voice 2 option (step 1)
        CRGB voice2Color = (uiState.settingsMenuIndex == 1) ?
            activeThemeColors->gateOnV2 : activeThemeColors->gateOffV2;
        if (uiState.settingsMenuIndex == 1) {
            // Add pulsing effect for selected option
            uint32_t time = millis();
            float pulse = 0.5f + 0.5f * sinf(time * 0.006f);
            voice2Color.nscale8(static_cast<uint8_t>(128 + 127 * pulse));
        } else {
            voice2Color.nscale8(96);
        }
        ledMatrix.setLED(1, 0, voice2Color);
    }
}

void updateVoiceParameterLEDs(LEDMatrix& ledMatrix, const UIState& uiState) {
    if (!uiState.inVoiceParameterMode) return;

    // Get active theme colors
    const LEDThemeColors* activeThemeColors = getActiveThemeColors();
    if (!activeThemeColors) return;

    // Clear all LEDs first
    for (int i = 0; i < LEDMatrix::WIDTH * LEDMatrix::HEIGHT; i++) {
        ledMatrix.setLED(i % LEDMatrix::WIDTH, i / LEDMatrix::WIDTH, CRGB::Black);
    }

    // Map button index to LED position (buttons 9-24 map to steps 8-23)
    uint8_t ledIndex = uiState.lastVoiceParameterButton - 1;
    if (ledIndex >= LEDMatrix::WIDTH * LEDMatrix::HEIGHT) return;

    // Choose color based on voice and parameter type
    CRGB paramColor;

    switch (uiState.lastVoiceParameterButton) {
        case 9:  // Envelope
            paramColor = uiState.isVoice2Mode ? activeThemeColors->modAttackActive : activeThemeColors->modDecayActive;
            break;
        case 10: // Overdrive
            paramColor = uiState.isVoice2Mode ? activeThemeColors->modFilterActive : activeThemeColors->modVelocityActive;
            break;
        case 11: // Wavefolder
            paramColor = uiState.isVoice2Mode ? activeThemeColors->modOctaveActive : activeThemeColors->modNoteActive;
            break;
        case 12: // Filter Mode
            paramColor = uiState.isVoice2Mode ? activeThemeColors->gateOnV2 : activeThemeColors->gateOnV1;
            break;
        case 13: // Filter Resonance
            paramColor = uiState.isVoice2Mode ? activeThemeColors->modSlideActive : activeThemeColors->modParamModeActive;
            break;
        default:
            paramColor = uiState.isVoice2Mode ? activeThemeColors->defaultActive : activeThemeColors->defaultInactive;
            break;
    }

    // Create pulsing effect for 3 seconds
    if (millis() - uiState.voiceParameterChangeTime < 3000) {
        uint32_t time = millis();
        float pulse = 0.5f + 0.5f * sinf(time * 0.01f); // Faster pulse for voice parameters
        paramColor.nscale8(static_cast<uint8_t>(128 + 127 * pulse));
    } else {
        paramColor.nscale8(64); // Dim after timeout
    }

    // Set the LED for the voice parameter button
    ledMatrix.setLED(ledIndex % LEDMatrix::WIDTH, ledIndex / LEDMatrix::WIDTH, paramColor);
}


/**
 * @brief Updates the LED matrix to reflect the current gate states of both sequencers.
 *
 * This function visually represents the gate status (on/off) for each step of two sequencers
 * on an LED matrix. It supports two main display modes:
 *   - **Idle/Breathing Mode:** When neither sequencer is running and no step is selected for editing,
 *     all step LEDs display a synchronized breathing animation.
 *   - **Active Sequencing Mode:** When either sequencer is running or a step is selected,
 *     each step's LED shows its gate state, slide activation, and playhead position for both voices.
 *
 * The function uses double-buffered color blending for smooth LED transitions and supports
 * highlighting slide and playhead states with accent colors.
 *
 * @param ledMatrix Reference to the LEDMatrix object controlling the physical LEDs.
 * @param seq1      Reference to the first Sequencer (Voice 1).
 * @param seq2      Reference to the second Sequencer (Voice 2).
 * @param uiState   Reference to the UIState struct containing UI and mode flags.
 */
// Render a voice pair (1/2) or (3/4) into the matrix
static void renderVoicePair(
    LEDMatrix& ledMatrix,
    const Sequencer& a,
    const Sequencer& b,
    const LEDThemeColors* theme,
    int baseOffset
) {
    // Debug assertions for sequencer validation
    uint8_t gateStepCountA = a.getParameterStepCount(ParamId::Gate);
    uint8_t gateStepCountB = b.getParameterStepCount(ParamId::Gate);
    
    if (gateStepCountA == 0) {
        DBG_WARN("renderVoicePair: Voice A has zero gate step count");
        return;
    }
    if (gateStepCountB == 0) {
        DBG_WARN("renderVoicePair: Voice B has zero gate step count");
        return;
    }

    for (int step = 0; step < 16; ++step) {
        // Top row voice (first voice in pair)
        const Step& sa = a.getStep(step);
        bool phA = (a.getCurrentStepForParameter(ParamId::Gate) == step && a.isRunning());
        CRGB colorA = sa.gate ? theme->gateOnV1 : theme->gateOffV1;
        if (a.getStepParameterValue(ParamId::Slide, step) > 0) { 
            nblend(colorA, theme->modSlideActive, 128); 
        }
        if (phA) { 
            colorA += theme->playheadAccent; 
        }
        
        // Fixed LED index calculation for top row
        int topRowIndex = baseOffset + step;
        nblend(smoothedTargetColors[topRowIndex], colorA, TARGET_SMOOTHING_BLEND_AMOUNT);
        nblend(ledMatrix.getLeds()[topRowIndex], smoothedTargetColors[topRowIndex], 166);

        // Bottom row voice (second voice in pair)
        const Step& sb = b.getStep(step);
        bool phB = (b.getCurrentStepForParameter(ParamId::Gate) == step && b.isRunning());
        CRGB colorB = sb.gate ? theme->gateOnV2 : theme->gateOffV2;
        if (b.getStepParameterValue(ParamId::Slide, step) > 0) { 
            nblend(colorB, theme->modSlideActive, 128); 
        }
        if (phB) { 
            colorB += theme->playheadAccent; 
        }
        
        // Fixed LED index calculation for bottom row 
        // For 8x8 matrix: bottom half starts at row 4 (y=4), which is index 32
        int bottomRowIndex = baseOffset + 32 + step; // 32 = 4 rows * 8 LEDs per row
        nblend(smoothedTargetColors[bottomRowIndex], colorB, TARGET_SMOOTHING_BLEND_AMOUNT);
        nblend(ledMatrix.getLeds()[bottomRowIndex], smoothedTargetColors[bottomRowIndex], 166);
    }
}
void updateGateLEDs(
    LEDMatrix& ledMatrix,
    const Sequencer& seq1,
    const Sequencer& seq2,
    const UIState& uiState
) {
    // --- Idle/Breathing Mode ---
    // If both sequencers are stopped and no step is selected for editing,
    // display a blue breathing effect on all step LEDs.
    if (!seq1.isRunning() && !seq2.isRunning() && uiState.selectedStepForEdit == -1) {
        float t = millis() / 5000.0f; // Slow time base for breathing
        float breath = 0.5f * (1.0f + sinf(2.0f * 3.1415926f * t)); // Sine wave [0,1]
        uint8_t b = (uint8_t)(breath * 64.0f + 16.0f); // Blue channel intensity

        for (int step = 0; step < 16; ++step) {
            CRGB currentTarget = CRGB(0, 0, b); // Blue breathing color

            // Blend target color into smoothed buffer for smooth transitions
            nblend(smoothedTargetColors[step], currentTarget, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 222);

            // Voice 2 bottom half - fixed offset calculation
            int ledIndex = 32 + step; // Bottom half of 8x8 matrix
            nblend(smoothedTargetColors[ledIndex], currentTarget, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 222);
        }
    } else {
        // --- Active Sequencing Mode ---
        // For each step, update the LED color based on gate, slide, and playhead status for both voices.
        for (int step = 0; step < 16; ++step) {
            // --- Voice 1 (seq1) ---
            const Step& s1 = seq1.getStep(step);
            bool isPlayhead1 = (seq1.getCurrentStepForParameter(ParamId::Gate) == step && seq1.isRunning());
            // Choose color based on gate state
            CRGB targetColor1 = s1.gate ? activeThemeColors->gateOnV1 : activeThemeColors->gateOffV1;

            // If slide is active for this step, blend in the slide accent color
            uint8_t slideValue1 = seq1.getStepParameterValue(ParamId::Slide, step);
            if (slideValue1 > 0) {
                nblend(targetColor1, activeThemeColors->modSlideActive, 128);
            }

            // If this step is the playhead, add the playhead accent color
            if (isPlayhead1) {
                targetColor1 += activeThemeColors->playheadAccent;
            }

            // Blend to smoothed buffer and then to the actual LED for smooth transitions
            nblend(smoothedTargetColors[step], targetColor1, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 166);

            // --- Voice 2 (seq2) ---
            const Step& s2 = seq2.getStep(step);
            bool isPlayhead2 = (seq2.getCurrentStepForParameter(ParamId::Gate) == step && seq2.isRunning());
            CRGB targetColor2 = s2.gate ? activeThemeColors->gateOnV2 : activeThemeColors->gateOffV2;

            uint8_t slideValue2 = seq2.getStepParameterValue(ParamId::Slide, step);
            if (slideValue2 > 0) {
                nblend(targetColor2, activeThemeColors->modSlideActive, 128);
            }

            if (isPlayhead2) {
                targetColor2 += activeThemeColors->playheadAccent;
            }

            int ledIndex = 32 + step; // Fixed offset for bottom half
            nblend(smoothedTargetColors[ledIndex], targetColor2, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 166);
        }
    }
}

void updateStepLEDs(
    LEDMatrix& ledMatrix,
    const Sequencer& seq1,
    const Sequencer& seq2,
    const Sequencer& seq3,
    const Sequencer& seq4,
    const UIState& uiState,
    int mm
) {
    // Handle settings mode LED feedback
    if (uiState.settingsMode) {
        updateSettingsModeLEDs(ledMatrix, uiState);
        return;
    }

    // Handle voice parameter mode LED feedback
    if (uiState.inVoiceParameterMode && (millis() - uiState.voiceParameterChangeTime < 3000)) {
        updateVoiceParameterLEDs(ledMatrix, uiState);
        return;
    }

    const ParamButtonMapping* heldMapping = getHeldParameterButton(uiState);
    bool anyParamForLengthHeld = (heldMapping != nullptr);
    ParamId activeParamIdForLength = anyParamForLengthHeld ? heldMapping->paramId : ParamId::Count;

    if (uiState.slideMode) {
        // Select sequencer based on selectedVoiceIndex (0..3)
        const Sequencer* seqPtr = (uiState.selectedVoiceIndex == 0) ? &seq1 :
                                  (uiState.selectedVoiceIndex == 1) ? &seq2 :
                                  (uiState.selectedVoiceIndex == 2) ? &seq3 : &seq4;
        const Sequencer& activeSeq = *seqPtr;
        uint8_t slidePlayhead = activeSeq.getCurrentStepForParameter(ParamId::Slide);
        uint8_t slideLength = activeSeq.getParameterStepCount(ParamId::Slide);

        for (int step = 0; step < NUMBER_OF_STEP_BUTTONS; step++) {
            uint8_t slideValue = activeSeq.getStepParameterValue(ParamId::Slide, step);
            bool isSlideActive = (slideValue > 0);
            bool isPlayhead = (step == slidePlayhead);
            bool isWithinLength = (step < slideLength);

            CRGB color;
            if (isPlayhead && isWithinLength) {
                color = activeThemeColors->modSlideActive;
            } else if (isSlideActive && isWithinLength) {
                color = activeThemeColors->modSlideActive;
                color.nscale8(64);
            } else if (isWithinLength) {
                color = activeThemeColors->modSlideInactive;
                color.nscale8(32);
            } else {
                color = CRGB::Black;
            }

            int x = step % LEDMatrix::WIDTH;
            int y = step / LEDMatrix::WIDTH;
            // Place on top/bottom half based on voice within pair (0/1 top, 2/3 page uses same rows)
            if ((uiState.selectedVoiceIndex % 2) == 1) {
                y += 3; // second voice in pair uses lower band
            }
            ledMatrix.setLED(x, y, color);
        }
        return;
    }

    bool paramValueEditActive = isAnyParameterButtonHeld(uiState);

    // Helper to fetch by selected voice
    auto& activeSeqRef = (uiState.selectedVoiceIndex == 0) ? seq1 :
                         (uiState.selectedVoiceIndex == 1) ? seq2 :
                         (uiState.selectedVoiceIndex == 2) ? seq3 : seq4;

    if (paramValueEditActive) {
        uint8_t currentLength = activeSeqRef.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = activeSeqRef.getCurrentStepForParameter(activeParamIdForLength);

        // Dim the non-selected row (top or bottom) in the current page
        bool isSecondInPair = (uiState.selectedVoiceIndex % 2) == 1;
        for (int step = 0; step < 16; ++step) {
            int topIndex = step;
            int bottomIndex = 32 + step; // Fixed offset for 8x8 matrix
            if (!isSecondInPair) {
                // Selected voice is top row; dim bottom
                nblend(smoothedTargetColors[bottomIndex], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
                nblend(ledMatrix.getLeds()[bottomIndex], smoothedTargetColors[bottomIndex], 32);
            } else {
                // Selected voice is bottom row; dim top
                nblend(smoothedTargetColors[topIndex], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
                nblend(ledMatrix.getLeds()[topIndex], smoothedTargetColors[topIndex], 32);
            }
        }

        // Paint the selected row with parameter length/playhead info
        for (int step = 0; step < 16; ++step) {
            CRGB targetColor;
            if (step < currentLength) {
                if (step == paramPlayhead && activeSeqRef.isRunning()) {
                    targetColor = getParameterColor(activeParamIdForLength, 180);
                } else {
                    // Use V1 tint for top row, V2 tint for bottom row
                    targetColor = isSecondInPair ? activeThemeColors->editModeDimBlueV2
                                                 : activeThemeColors->editModeDimBlueV1;
                }
            } else {
                targetColor = CRGB::Black;
            }
            int ledIndex = (isSecondInPair ? 32 : 0) + step; // Fixed offset
            nblend(smoothedTargetColors[ledIndex], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], isSecondInPair ? 122 : 64);
        }

        return;
    }

    if (anyParamForLengthHeld) {
        uint8_t currentLength = activeSeqRef.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = activeSeqRef.getCurrentStepForParameter(activeParamIdForLength);

        // Paint only the selected row's within-length area
        bool isSecondInPair = (uiState.selectedVoiceIndex % 2) == 1;
        for (int step = 0; step < currentLength; ++step) {
            CRGB targetColor = (step == paramPlayhead && activeSeqRef.isRunning())
                                   ? getParameterColor(activeParamIdForLength, 180)
                                   : (isSecondInPair ? activeThemeColors->editModeDimBlueV2
                                                     : activeThemeColors->editModeDimBlueV1);
            int ledIndex = (isSecondInPair ? 32 : 0) + step; // Fixed offset
            nblend(smoothedTargetColors[ledIndex], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], isSecondInPair ? 200 : 60);
        }

        // Dim the other row's within-length area
        for (int step = 0; step < currentLength; ++step) {
            int otherIndex = (isSecondInPair ? 0 : 32) + step; // Fixed offset
            nblend(smoothedTargetColors[otherIndex], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[otherIndex], smoothedTargetColors[otherIndex], 150);
        }
    } else {
        // Determine which voice pair to display based on selectedVoiceIndex
        bool showFirstPair = (uiState.selectedVoiceIndex < 2);
        bool secondInPair = (uiState.selectedVoiceIndex % 2) == 1;
        const LEDThemeColors* theme = getActiveThemeColors();

        // Clear first to avoid ghosting when switching pages
        for (int i = 0; i < LEDMatrix::WIDTH * LEDMatrix::HEIGHT; ++i) {
            nblend(smoothedTargetColors[i], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[i], smoothedTargetColors[i], 64);
        }

        // Render either voices 1/2 (page 1) or 3/4 (page 2)
        if (showFirstPair) {
            renderVoicePair(ledMatrix, seq1, seq2, theme, 0);
        } else {
            renderVoicePair(ledMatrix, seq3, seq4, theme, 0);
        }

        // Polyrhythmic overlays for the visible pair only
        if (showFirstPair) {
            addPolyrhythmicOverlay(ledMatrix, seq1, false, 32);
            addPolyrhythmicOverlay(ledMatrix, seq2, true, 32);
        } else {
            addPolyrhythmicOverlay(ledMatrix, seq3, false, 32);
            addPolyrhythmicOverlay(ledMatrix, seq4, true, 32);
        }

        // Corner indicator for page: top-left = page1 (1/2), top-right = page2 (3/4)
        if (showFirstPair) {
            ledMatrix.setLED(0, 0, CRGB(0, 15, 0)); // small green dot
        } else {
            ledMatrix.setLED(LEDMatrix::WIDTH - 1, 0, CRGB(0, 15, 15)); // cyan dot
        }

        // Highlight selected step if editing
        if (uiState.selectedStepForEdit >= 0 && uiState.selectedStepForEdit < 16) {
            int ledIndex = uiState.selectedVoiceIndex % 2 == 1 ? (32 + uiState.selectedStepForEdit) : uiState.selectedStepForEdit; // Fixed offset

            static bool blinkState = false;
            static uint32_t lastBlinkTime = 0;
            uint32_t currentTime = millis();
            if (currentTime - lastBlinkTime > 500) {
                blinkState = !blinkState;
                lastBlinkTime = currentTime;
            }

            CRGB highlightColor = blinkState ? CRGB::White : CRGB::Black;
            nblend(smoothedTargetColors[ledIndex], highlightColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 100);
        }
    }
}
