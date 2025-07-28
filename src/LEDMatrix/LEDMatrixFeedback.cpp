#include <Arduino.h>
#include "LEDMatrixFeedback.h"
#include <FastLED.h>
#include <cmath>

#include "ledMatrix.h"
#include "../sequencer/Sequencer.h"
#include "../ui/UIEventHandler.h"
#include "../ui/ButtonManager.h"

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
    bool isVoice2Mode,
    uint8_t intensity = 32
) {
    if (!seq.isRunning()) return;

    const int baseOffset = isVoice2Mode ? ledOffset : 0;
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

            // Calculate LED coordinates
            // Calculate LED coordinates from linear paramStep index
            const int x = paramStep % LEDMatrix::WIDTH;
            // Row index, shifted down by 3 if isVoice2Mode is active
            const int y = paramStep / LEDMatrix::WIDTH + (isVoice2Mode ? 3 : 0);

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

            // Repeat for Voice 2 (offset in LED matrix)
            int ledIndex = ledOffset + step;
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

            int ledIndex = ledOffset + step;
            nblend(smoothedTargetColors[ledIndex], targetColor2, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 166);
        }
    }
}

void updateStepLEDs(
    LEDMatrix& ledMatrix,
    const Sequencer& seq1,
    const Sequencer& seq2,
    const UIState& uiState,
    int mm
) {
    const ParamButtonMapping* heldMapping = getHeldParameterButton(uiState);
    bool anyParamForLengthHeld = (heldMapping != nullptr);
    ParamId activeParamIdForLength = anyParamForLengthHeld ? heldMapping->paramId : ParamId::Count;

    if (uiState.slideMode) {
        const Sequencer& activeSeq = uiState.isVoice2Mode ? seq2 : seq1;
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
            if (uiState.isVoice2Mode) {
                y += 3;
            }
            ledMatrix.setLED(x, y, color);
        }
        return;
    }

    bool paramValueEditActive = isAnyParameterButtonHeld(uiState);

    if (paramValueEditActive && !uiState.isVoice2Mode) {
        uint8_t currentLength = seq1.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = seq1.getCurrentStepForParameter(activeParamIdForLength);

        for (int step = 0; step < 16; ++step) {
            CRGB targetColor;
            if (step < currentLength) {
                if (step == paramPlayhead && seq1.isRunning()) {
                    targetColor = getParameterColor(activeParamIdForLength, 180);
                } else {
                    targetColor = activeThemeColors->editModeDimBlueV1;
                }
            } else {
                targetColor = CRGB::Black;
            }
            nblend(smoothedTargetColors[step], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 64);
        }

        for (int step = 0; step < 16; ++step) {
            int ledIndex = ledOffset + step;
            nblend(smoothedTargetColors[ledIndex], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 32);
        }

        return;
    }
    if (paramValueEditActive && uiState.isVoice2Mode) {
        uint8_t currentLength = seq2.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = seq2.getCurrentStepForParameter(activeParamIdForLength);

        for (int step = 0; step < 16; ++step) {
            nblend(smoothedTargetColors[step], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 32);
        }

        for (int step = 0; step < 16; ++step) {
            CRGB targetColor;
            if (step < currentLength) {
                if (step == paramPlayhead && seq2.isRunning()) {
                    targetColor = getParameterColor(activeParamIdForLength, 180);
                } else {
                    targetColor = activeThemeColors->editModeDimBlueV2;
                }
            } else {
                targetColor = CRGB::Black;
            }
            int ledIndex = ledOffset + step;
            nblend(smoothedTargetColors[ledIndex], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 122);
        }
        return;
    }

    if (!uiState.isVoice2Mode && anyParamForLengthHeld) {
        uint8_t currentLength = seq1.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = seq1.getCurrentStepForParameter(activeParamIdForLength);
        for (int step = 0; step < currentLength; ++step) {
            CRGB targetColor;
            if (step < currentLength) {
                if (step == paramPlayhead && seq1.isRunning()) {
                    targetColor = getParameterColor(activeParamIdForLength, 180);
                } else {
                    targetColor = activeThemeColors->editModeDimBlueV1;
                }
            } else {
                targetColor = CRGB::Black;
            }
            nblend(smoothedTargetColors[step], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 60);
        }
        for (int step = 0; step < currentLength; ++step) {
            nblend(smoothedTargetColors[ledOffset + step], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledOffset + step], smoothedTargetColors[ledOffset + step], 150);
        }
    } else if (uiState.isVoice2Mode && anyParamForLengthHeld) {
        uint8_t currentLength = seq2.getParameterStepCount(activeParamIdForLength);
        uint8_t paramPlayhead = seq2.getCurrentStepForParameter(activeParamIdForLength);
        for (int step = 0; step < currentLength; ++step) {
            nblend(smoothedTargetColors[step], CRGB::Black, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[step], smoothedTargetColors[step], 200);
        }
        for (int step = 0; step < currentLength; ++step) {
            CRGB targetColor;
            if (step < currentLength) {
                if (step == paramPlayhead && seq2.isRunning()) {
                    targetColor = getParameterColor(activeParamIdForLength, 180);
                } else {
                    targetColor = activeThemeColors->editModeDimBlueV2;
                }
            } else {
                targetColor = CRGB::Black;
            }
            int ledIndex = ledOffset + step;
            nblend(smoothedTargetColors[ledIndex], targetColor, TARGET_SMOOTHING_BLEND_AMOUNT);
            nblend(ledMatrix.getLeds()[ledIndex], smoothedTargetColors[ledIndex], 200);
        }
    } else {
        updateGateLEDs(ledMatrix, seq1, seq2, uiState);

        addPolyrhythmicOverlay(ledMatrix, seq1, false, 32);
        addPolyrhythmicOverlay(ledMatrix, seq2, true, 32);

        if (uiState.selectedStepForEdit >= 0 && uiState.selectedStepForEdit < 16) {
            int ledIndex = uiState.isVoice2Mode ? (ledOffset + uiState.selectedStepForEdit) : uiState.selectedStepForEdit;

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
