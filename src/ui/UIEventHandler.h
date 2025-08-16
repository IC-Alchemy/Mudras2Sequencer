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
#include "ButtonHandlers.h"
#include "UIConstants.h"

// Forward declarations to prevent circular dependencies
class Sequencer;
class MidiNoteManager; // Forward declare MidiNoteManager

// =======================
//   CONSTANTS
// =======================
// Moved to UIConstants.h to centralize button mappings, timing, and layout.

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
