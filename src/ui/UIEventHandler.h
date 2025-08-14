#ifndef UI_EVENT_HANDLER_H
#define UI_EVENT_HANDLER_H

#include <Arduino.h>
#include <stddef.h>
#include "../matrix/Matrix.h"
#include "../sequencer/SequencerDefs.h"
#include "../sensors/as5600.h"
#include "../LEDMatrix/LEDMatrixFeedback.h"
#include "ButtonManager.h"
#include "UIState.h"

// Forward declarations to prevent circular dependencies
class Sequencer;
class MidiNoteManager; // Forward declare MidiNoteManager

// =======================
//   CONSTANTS
// =======================

// Button indices for different functions
constexpr uint8_t BUTTON_SLIDE_MODE = 22;
constexpr uint8_t BUTTON_TOGGLE_DELAY = 23;
constexpr uint8_t BUTTON_VOICE_SWITCH = 24;
constexpr uint8_t BUTTON_AS5600_CONTROL = 25;
constexpr uint8_t BUTTON_PLAY_STOP = 26;
constexpr uint8_t BUTTON_CHANGE_SCALE = 27;
constexpr uint8_t BUTTON_CHANGE_THEME = 28;
constexpr uint8_t BUTTON_CHANGE_SWING_PATTERN = 29;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ1 = 30;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ2 = 31;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ3 = 32;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ4 = 33;

constexpr uint8_t NUMBER_OF_STEP_BUTTONS = 16;
constexpr unsigned long AS5600_DOUBLE_PRESS_WINDOW = 300; // ms
constexpr unsigned long CONTROL_LED_FLASH_DURATION_MS = 250;

// =======================
//   FUNCTION DECLARATIONS
// =======================

/**
 * @brief Initialize the UI event handler system.
 * @param uiState Reference to the central UI state object.
 */
void initUIEventHandler(UIState& uiState);

/**
 * @brief Main matrix event handler (Arduino-friendly consolidated signature).
 *        Accepts an array of Sequencer* plus count to support any number of voices.
 *
 * @param evt             Matrix button event (button index and press/release type).
 * @param uiState         Central UI state object (mutable).
 * @param sequencers      Array of non-owning Sequencer* pointers. Must have at least two
 *                        entries for full functionality; additional entries are allowed.
 * @param sequencerCount  Number of entries in the sequencers array.
 * @param midiNoteManager MIDI note lifecycle manager for note on/off and CC handling.
 */
void matrixEventHandler(const MatrixButtonEvent& evt,
                        UIState& uiState,
                        Sequencer* const* sequencers,
                        size_t sequencerCount,
                        MidiNoteManager& midiNoteManager);

void pollUIHeldButtons(UIState& uiState, Sequencer& seq1, Sequencer& seq2);

// Compatibility overloads (will forward to consolidated handler)
void matrixEventHandler(const MatrixButtonEvent &evt, UIState& uiState, Sequencer& seq1, Sequencer& seq2, MidiNoteManager& midiNoteManager);
void matrixEventHandler(const MatrixButtonEvent &evt, UIState& uiState, Sequencer& seq1, Sequencer& seq2, Sequencer& seq3, Sequencer& seq4, MidiNoteManager& midiNoteManager);
void pollUIHeldButtons(UIState& uiState, Sequencer& seq1, Sequencer& seq2, Sequencer& seq3, Sequencer& seq4);

#endif // UI_EVENT_HANDLER_H
