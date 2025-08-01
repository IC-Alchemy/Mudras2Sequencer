#ifndef LEDMATRIX_FEEDBACK_H
#define LEDMATRIX_FEEDBACK_H

#include "ledMatrix.h"

// Forward declarations to break circular dependencies
class Sequencer;
struct UIState;

/**
 * @brief Updates the step LEDs based on sequencer and UI state.
 */
void updateStepLEDs(
    LEDMatrix& ledMatrix,
    const Sequencer& seq1,
    const Sequencer& seq2,
    const UIState& uiState,
    int mm
);


void setStepLedColor(uint8_t step, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Initializes the LED matrix feedback system, including smoothed target colors.
 */
void setupLEDMatrixFeedback();

// Enum for different color themes
enum class LEDTheme {
    DEFAULT = 0,
    OCEANIC,
    VOLCANIC,
    FOREST,
    NEON,
    COUNT // Keep this last to get the number of themes
};

// Structure to hold a set of theme colors
struct LEDThemeColors {
    CRGB gateOnV1;
    CRGB gateOffV1;
    CRGB playheadAccent;
    CRGB gateOnV2;
    CRGB gateOffV2;
    CRGB idleBreathingBlue;
    CRGB editModeDimBlueV1;
    CRGB editModeDimBlueV2;
    CRGB modNoteActive;
    CRGB modNoteInactive;
    CRGB modVelocityActive;
    CRGB modVelocityInactive;
    CRGB modFilterActive;
    CRGB modFilterInactive;
    CRGB modDecayActive;
    CRGB modDecayInactive;
    CRGB modAttackActive;
    CRGB modAttackInactive;
    CRGB modOctaveActive;
    CRGB modOctaveInactive;
    CRGB modSlideActive;
    CRGB modSlideInactive;
    CRGB defaultActive;
    CRGB defaultInactive;
    CRGB modParamModeActive;
    CRGB modParamModeInactive;
    CRGB modGateModeActive;
    CRGB modGateModeInactive;
    CRGB randomizeFlash;
    CRGB randomizeIdle;
};

/**
 * @brief Sets the active LED color theme.
 * @param theme The LEDTheme to activate.
 */
void setLEDTheme(LEDTheme theme);

/**
 * @brief Get a pointer to the currently active theme colors.
 */
const LEDThemeColors* getActiveThemeColors();

#endif // LEDMATRIX_FEEDBACK_H
