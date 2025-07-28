#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "../sequencer/SequencerDefs.h"
#include "UIState.h" // Include the new state header

/**
 * @brief Button state and timing management for PicoMudrasSequencer UI
 * 
 * Provides utilities for button press detection and parameter button mappings,
 * operating on a central UIState object.
 */

// Long press detection threshold
constexpr unsigned long LONG_PRESS_THRESHOLD = 400; // ms

// =======================
//   BUTTON MAPPING STRUCTURES
// =======================

/**
 * @brief Complete button mapping with index, parameter, and name.
 * The held state is now managed in UIState, not via a pointer.
 */
struct ParamButtonMapping {
    uint8_t buttonIndex;
    ParamId paramId;
    const char *name;
};

// =======================
//   BUTTON MAPPING ARRAY
// =======================

// Complete parameter button mappings
extern const ParamButtonMapping PARAM_BUTTON_MAPPINGS[];
extern const size_t PARAM_BUTTON_MAPPINGS_SIZE;

// =======================
//   FUNCTION DECLARATIONS
// =======================

/**
 * @brief Initialize the UI state for the button manager.
 * @param uiState Reference to the central UI state object.
 */
void initButtonManager(UIState& uiState);

/**
 * @brief Check if a press duration qualifies as a long press.
 * @param pressDuration Duration of button press in milliseconds.
 * @return true if duration exceeds long press threshold.
 */
bool isLongPress(unsigned long pressDuration);

/**
 * @brief Check if any parameter button is currently held.
 * @param uiState Const reference to the central UI state object.
 * @return true if any parameter button is held.
 */
bool isAnyParameterButtonHeld(const UIState& uiState);

/**
 * @brief Get the mapping for the currently held parameter button.
 * @param uiState Const reference to the central UI state object.
 * @return Pointer to the held parameter mapping, or nullptr if none held.
 */
const ParamButtonMapping* getHeldParameterButton(const UIState& uiState);

#endif // BUTTON_MANAGER_H
