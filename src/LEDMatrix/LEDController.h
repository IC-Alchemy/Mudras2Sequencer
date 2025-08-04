#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "../ui/UIState.h"      // For UIState
#include "../sensors/as5600.h"     // For AS5600Sensor
#include "ledMatrix.h"             // For LEDMatrix class
#include "LEDMatrixFeedback.h"     // For LEDThemeColors

/**
 * @brief LED Controller for PicoMudrasSequencer
 * 
 * Manages all control LED functionality including parameter button feedback,
 * mode indicators, voice selection, and AS5600 encoder visual feedback.
 * Consolidates LED control logic while maintaining real-time performance.
 */

// =======================
//   LED INDEX CONSTANTS
// =======================

namespace ControlLEDs
{
    // Parameter button LEDs (buttons 16-22)
    constexpr int NOTE_LED = 48;
    constexpr int VELOCITY_LED = 49;
    constexpr int FILTER_LED = 50;
    constexpr int ATTACK_LED = 51;
    constexpr int DECAY_LED = 52;
    constexpr int OCTAVE_LED = 53;
    constexpr int SLIDE_LED = 54;

    // Delay parameter LEDs (AS5600 encoder control)
    constexpr int DELAY_TIME_LED = 40;      // Position (0,6)
    constexpr int DELAY_FEEDBACK_LED = 41;  // Position (0,7)


    // Mode indicator LEDs
    constexpr int VOICE1_LED = 56;
    constexpr int VOICE2_LED = 57;
    constexpr int DELAY_TOGGLE_LED = 59;  // Delay effect on/off indicator
    constexpr int RANDOMIZE_LED = 64;

    // Timing constants for LED animations
    constexpr float PULSE_FREQUENCY = 0.006f;
    constexpr uint8_t PULSE_BASE_BRIGHTNESS = 22;
    constexpr uint8_t PULSE_AMPLITUDE = 188;
}

// =======================
//   EXTERNAL DEPENDENCIES
// =======================


// External function declarations
extern float getAS5600ParameterValue();
extern const LEDThemeColors* getActiveThemeColors();

// =======================
//   FUNCTION DECLARATIONS
// =======================

/**
 * @brief Update all control LEDs with current state
 * 
 * @param ledMatrix Reference to the LED matrix for output
 * @param uiState Const reference to the central UI state object.
 */
void updateControlLEDs(LEDMatrix &ledMatrix, const UIState& uiState);

/**
 * @brief Initialize the LED controller
 */
void initLEDController();

#endif // LED_CONTROLLER_H
