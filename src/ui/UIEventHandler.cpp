#include "UIEventHandler.h"
#include "../midi/MidiManager.h"
#include "../scales/scales.h"
#include "../sequencer/Sequencer.h"
#include "../sequencer/ShuffleTemplates.h"
#include "../voice/Voice.h"
#include "ButtonManager.h"
#include <uClock.h>

// External function declarations that the UI calls
extern void onClockStart();
extern void onClockStop();
extern void setLEDTheme(LEDTheme theme);
extern void applyVoicePreset(uint8_t voiceNumber, uint8_t presetIndex);

// External variables that are still needed from the main file
extern uint8_t currentScale;
extern bool isClockRunning;
extern const ParameterDefinition CORE_PARAMETERS[];

// Helper function declarations (static to this file)
static bool handleParameterButtonEvent(const MatrixButtonEvent &evt,
                                       UIState &uiState);
static bool handleStepButtonEvent(const MatrixButtonEvent &evt,
                                  UIState &uiState, Sequencer &seq1,
                                  Sequencer &seq2);
static void handleLFOAssignment(uint8_t buttonIndex, UIState &uiState,
                                Sequencer &seq1, Sequencer &seq2);
static void autoSelectAS5600Parameter(ParamId paramId, UIState &uiState);
static void handleAS5600ParameterControl(UIState &uiState);
static void handleControlButtonEvent(uint8_t buttonIndex, UIState &uiState,
                                     Sequencer &seq1, Sequencer &seq2);

void initUIEventHandler(UIState &uiState) {
  initButtonManager(uiState);

  // Initialize default voice preset indices
  uiState.voice1PresetIndex = 0; // Analog preset
  uiState.voice2PresetIndex = 1; // Digital preset
}

void matrixEventHandler(const MatrixButtonEvent &evt, UIState &uiState,
                        Sequencer &seq1, Sequencer &seq2,
                        MidiNoteManager &midiNoteManager) {
  // Check and trigger any pending long-press resets immediately
  pollUIHeldButtons(uiState, seq1, seq2);

  // Handle LFO assignment mode
  if (uiState.lfoAssignMode) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      handleLFOAssignment(evt.buttonIndex, uiState, seq1, seq2);
      uiState.lfoAssignMode = false;
      Serial.println("Exited LFO assignment mode");
    }
    return; // Exit after handling
  }
  // Handle Slide Mode toggle (Button 22)
  if (evt.buttonIndex == BUTTON_SLIDE_MODE) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      unsigned long now = millis();
      const unsigned long SLIDE_MODE_DEBOUNCE_MS = 150;
      if (now - uiState.lastSlideModeToggleTime >= SLIDE_MODE_DEBOUNCE_MS) {
        uiState.lastSlideModeToggleTime = now;
        // Toggle slide mode on press
        uiState.slideMode = !uiState.slideMode;
        if (uiState.slideMode) {
          // Clear conflicting modes when entering slide mode
          for (int i = 0; i < PARAM_ID_COUNT; ++i) {
            uiState.parameterButtonHeld[i] = false;
          }
          uiState.modGateParamSeqLengthsMode = false;
          uiState.selectedStepForEdit = -1;
          Serial.println("Entered Slide Mode");
        } else {
          Serial.println("Exited Slide Mode");
        }
      }
    }
    return; // Exit after handling
  }

  // Handle Voice Switch (Button 24) with long press for LFO mode
  if (evt.buttonIndex == BUTTON_VOICE_SWITCH) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      uiState.button24PressTime = millis();
      uiState.button24WasPressed = true;
    } else if (evt.type == MATRIX_BUTTON_RELEASED &&
               uiState.button24WasPressed) {
      unsigned long pressDuration = millis() - uiState.button24PressTime;
      uiState.button24WasPressed = false;

      if (isLongPress(pressDuration)) {
        uiState.lfoAssignMode = true;
        Serial.print("Entered LFO assignment mode for ");
        Serial.println(uiState.isVoice2Mode ? "LFO2 (Voice 2)"
                                            : "LFO1 (Voice 1)");
      } else {
        midiNoteManager.onModeSwitch();
        uiState.isVoice2Mode = !uiState.isVoice2Mode;
        uiState.selectedStepForEdit = -1;
        Serial.print("Switched to Voice ");
        Serial.println(uiState.isVoice2Mode ? "2" : "1");
      }
    }
    return; // Exit after handling
  }

  // --- Handle step buttons in slide mode ---
  if (uiState.slideMode && evt.buttonIndex < NUMBER_OF_STEP_BUTTONS) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      Sequencer &currentActiveSeq = uiState.isVoice2Mode ? seq2 : seq1;
      uint8_t currentSlideValue = currentActiveSeq.getStepParameterValue(
          ParamId::Slide, evt.buttonIndex);
      uint8_t newSlideValue = (currentSlideValue > 0) ? 0 : 1;
      currentActiveSeq.setStepParameterValue(ParamId::Slide, evt.buttonIndex,
                                             newSlideValue);
      Serial.print("Step ");
      Serial.print(evt.buttonIndex);
      Serial.print(" slide ");
      Serial.println(newSlideValue > 0 ? "ON" : "OFF");
    }
    return; // In slide mode, step buttons only toggle slide.
  }

  // Handle other buttons
  if (handleParameterButtonEvent(evt, uiState))
    return;
  if (handleStepButtonEvent(evt, uiState, seq1, seq2))
    return;

  // Handle Randomize 1 button: short press randomizes, long press resets
  // immediately
  if (evt.buttonIndex == BUTTON_RANDOMIZE_SEQ1) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      // Record press start time
      uiState.randomize1PressTime = millis();
      uiState.randomize1WasPressed = true;
    } else if (evt.type == MATRIX_BUTTON_RELEASED) {
      // Calculate press duration
      unsigned long heldTime = millis() - uiState.randomize1PressTime;
      if (!isLongPress(heldTime)) {
        // Short press: randomize parameters
        seq1.randomizeParameters();
        Serial.println("Seq 1 randomized by short press");
      }
      // Reset button state flags
      uiState.randomize1WasPressed = false;
      uiState.randomize1ResetTriggered = false;
      // Common UI updates
      uiState.selectedStepForEdit = -1;
      uiState.flash31Until = millis() + CONTROL_LED_FLASH_DURATION_MS;
    }
    return; // Exit after handling
  }

  // Handle Randomize 2 button: short press randomizes, long press resets
  // immediately
  if (evt.buttonIndex == BUTTON_RANDOMIZE_SEQ2) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      // Record press start time
      uiState.randomize2PressTime = millis();
      uiState.randomize2WasPressed = true;
    } else if (evt.type == MATRIX_BUTTON_RELEASED) {
      // Calculate press duration
      unsigned long heldTime = millis() - uiState.randomize2PressTime;
      if (!isLongPress(heldTime)) {
        // Short press: randomize parameters
        seq2.randomizeParameters();
        Serial.println("Seq 2 randomized by short press");
      }
      // Reset button state flags
      uiState.randomize2WasPressed = false;
      uiState.randomize2ResetTriggered = false;
      // Common UI updates
      uiState.selectedStepForEdit = -1;
      uiState.flash31Until = millis() + CONTROL_LED_FLASH_DURATION_MS;
    }
    return; // Exit after handling
  }

  // Handle PLAY_STOP button with long press detection for settings mode
  if (evt.buttonIndex == BUTTON_PLAY_STOP) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      uiState.playStopPressTime = millis();
      uiState.playStopWasPressed = true;
    } else if (evt.type == MATRIX_BUTTON_RELEASED &&
               uiState.playStopWasPressed) {
      unsigned long pressDuration = millis() - uiState.playStopPressTime;
      uiState.playStopWasPressed = false;

      if (isLongPress(pressDuration) && !isClockRunning) {
        // Long press when stopped: enter settings mode
        uiState.settingsMode = true;
        uiState.settingsMenuIndex = 0;
        uiState.settingsSubMenuIndex = 0;
        uiState.inPresetSelection = false;
        Serial.println("Entered settings mode");
      } else if (!isLongPress(pressDuration)) {
        // Short press: normal play/stop functionality
        handleControlButtonEvent(evt.buttonIndex, uiState, seq1, seq2);
      }
    }
    return;
  }

  // Handle other control buttons (only on press)
  if (evt.type == MATRIX_BUTTON_PRESSED) {
    handleControlButtonEvent(evt.buttonIndex, uiState, seq1, seq2);
  }
}

// =======================
//   INTERNAL HANDLERS
// =======================

/**
 * @brief Handles parameter button events from the matrix button grid.
 *
 * This function checks if the given button event corresponds to any parameter
 * button mapping. If a match is found, it updates the UI state to reflect
 * whether the parameter button is held, prints debug information to the serial
 * output, and, if the button is pressed and not the "Note" parameter, selects
 * the AS5600 for the corresponding parameter.
 *
 * @param evt      The MatrixButtonEvent containing the button index and event
 * type (pressed/released).
 * @param uiState  Reference to the UIState object to update the held state of
 * parameter buttons.
 * @return true if the event was handled as a parameter button event; false
 * otherwise.
 */
static bool handleParameterButtonEvent(const MatrixButtonEvent &evt,
                                       UIState &uiState) {
  // Block parameter button handling in slide mode
  if (uiState.slideMode) {
    return false;
  }
  for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
    const auto &mapping = PARAM_BUTTON_MAPPINGS[i];
    if (evt.buttonIndex == mapping.buttonIndex) {
      bool pressed = (evt.type == MATRIX_BUTTON_PRESSED);
      uiState.parameterButtonHeld[static_cast<int>(mapping.paramId)] = pressed;

      Serial.print("Button ");
      Serial.print(mapping.buttonIndex);
      Serial.print(" (");
      Serial.print(mapping.name);
      Serial.print(") ");
      Serial.println(pressed ? "pressed" : "released");

      // Automatically select AS5600 parameter if not the Note parameter and
      // button is pressed
      if (pressed && mapping.paramId != ParamId::Note) {
        autoSelectAS5600Parameter(mapping.paramId, uiState);
      }
      return true;
    }
  }
  return false;
}

static bool handleStepButtonEvent(const MatrixButtonEvent &evt,
                                  UIState &uiState, Sequencer &seq1,
                                  Sequencer &seq2) {
  // Ignore out-of-bounds button indices
  if (evt.buttonIndex >= NUMBER_OF_STEP_BUTTONS) {
    return false;
  }

  // Handle settings mode navigation
  if (uiState.settingsMode && evt.type == MATRIX_BUTTON_PRESSED) {
    if (uiState.inPresetSelection) {
      // In preset selection mode
      if (evt.buttonIndex < VoicePresets::getPresetCount()) {
        // Select and apply preset
        if (uiState.settingsMenuIndex == 0) {
          uiState.voice1PresetIndex = evt.buttonIndex;
          applyVoicePreset(1, evt.buttonIndex);
          Serial.print("Voice 1 preset set to: ");
          Serial.println(VoicePresets::getPresetName(evt.buttonIndex));
        } else if (uiState.settingsMenuIndex == 1) {
          uiState.voice2PresetIndex = evt.buttonIndex;
          applyVoicePreset(2, evt.buttonIndex);
          Serial.print("Voice 2 preset set to: ");
          Serial.println(VoicePresets::getPresetName(evt.buttonIndex));
        }
        uiState.inPresetSelection = false;
      }
    } else {
      // Main settings menu navigation
      if (evt.buttonIndex < 2) { // Only Voice 1 and Voice 2 options
        uiState.settingsMenuIndex = evt.buttonIndex;
        uiState.inPresetSelection = true;
        Serial.print("Entered preset selection for Voice ");
        Serial.println(evt.buttonIndex + 1);
      }
    }
    return true;
  }

  // Select current active sequencer based on voice mode
  Sequencer &currentActiveSeq = uiState.isVoice2Mode ? seq2 : seq1;

  // --- If holding any parameter button, pressing a step button will adjust
  // length of corresponding parameter ---
  if (isAnyParameterButtonHeld(uiState) && evt.type == MATRIX_BUTTON_PRESSED) {
    const ParamButtonMapping *heldMapping = getHeldParameterButton(uiState);
    if (heldMapping) {
      uint8_t newStepCount = evt.buttonIndex + 1;
      currentActiveSeq.setParameterStepCount(heldMapping->paramId,
                                             newStepCount);
      Serial.print("Set ");
      Serial.print(heldMapping->name);
      Serial.print(" parameter length to ");
      Serial.println(newStepCount);
    }
    return true;
  }

  // --- Normal mode: differentiate between short/long press for
  // edit/select/toggle ---
  if (!isAnyParameterButtonHeld(uiState)) {
    if (evt.type == MATRIX_BUTTON_PRESSED) {
      // Record press time for this pad
      uiState.padPressTimestamps[evt.buttonIndex] = millis();
    } else if (evt.type == MATRIX_BUTTON_RELEASED) {
      unsigned long pressDuration =
          millis() - uiState.padPressTimestamps[evt.buttonIndex];
      uiState.padPressTimestamps[evt.buttonIndex] = 0;

      if (isLongPress(pressDuration)) {
        // Toggle step selection for editing
        uiState.selectedStepForEdit =
            (uiState.selectedStepForEdit == evt.buttonIndex) ? -1
                                                             : evt.buttonIndex;
      } else {
        // Short press toggles step (on/off) and exits edit mode
        currentActiveSeq.toggleStep(evt.buttonIndex);
        uiState.selectedStepForEdit = -1;
      }
    }
  }
  return true;
}

static void handleControlButtonEvent(uint8_t buttonIndex, UIState &uiState,
                                     Sequencer &seq1, Sequencer &seq2) {
  switch (buttonIndex) {
  case BUTTON_SLIDE_MODE:
    uiState.slideMode = !uiState.slideMode;
    uiState.selectedStepForEdit = -1;
    Serial.print("Slide mode ");
    Serial.println(uiState.slideMode ? "ON" : "OFF");
    break;
  case BUTTON_AS5600_CONTROL:
    handleAS5600ParameterControl(uiState);
    break;
  case BUTTON_PLAY_STOP:
    if (isClockRunning) {
      onClockStop();
      //  Settings mode should turn on here
      uiState.settingsMode = true;
    } else {
      onClockStart();
      //  Settings mode should turn off here
      if (uiState.settingsMode) {
        uiState.settingsMode = false;
        uiState.inPresetSelection = false;
        Serial.println("Exited settings mode");
      }
      uiState.flash25Until = millis() + CONTROL_LED_FLASH_DURATION_MS;
    }
  break;
case BUTTON_CHANGE_SCALE:
  currentScale = (currentScale + 1) % 7;
  Serial.print("Scale changed to: ");
  Serial.print(currentScale);
  Serial.print(" (");
  Serial.print(scaleNames[currentScale]);
  Serial.println(")");
  break;
case BUTTON_CHANGE_THEME:
  uiState.currentThemeIndex =
      (uiState.currentThemeIndex + 1) % static_cast<int>(LEDTheme::COUNT);
  setLEDTheme(static_cast<LEDTheme>(uiState.currentThemeIndex));
  break;
case BUTTON_CHANGE_SWING_PATTERN:
  uiState.currentShufflePatternIndex =
      (uiState.currentShufflePatternIndex + 1) % NUM_SHUFFLE_TEMPLATES;
  {
    const ShuffleTemplate &currentTemplate =
        shuffleTemplates[uiState.currentShufflePatternIndex];

    // Apply shuffle template to uClock
    uClock.setShuffleTemplate(const_cast<int8_t *>(currentTemplate.ticks),
                              SHUFFLE_TEMPLATE_SIZE);
    uClock.setShuffle(uiState.currentShufflePatternIndex >
                      0); // Enable shuffle if not "No Shuffle"

    Serial.print("Shuffle pattern changed to index ");
    Serial.print(uiState.currentShufflePatternIndex);
    Serial.print(": ");
    Serial.println(currentTemplate.name);
  }
  break;

case BUTTON_TOGGLE_DELAY:
  uiState.delayOn = !uiState.delayOn;
  uiState.flash23Until = millis() + CONTROL_LED_FLASH_DURATION_MS;
  if (uiState.delayOn) {
    uiState.currentAS5600Parameter = AS5600ParameterMode::DelayTime;
    Serial.println("Delay ON - AS5600 set to Delay Time");
  } else {
    Serial.println("Delay OFF");
  }
  break;
}
}

static void handleLFOAssignment(uint8_t buttonIndex, UIState &uiState,
                                Sequencer &seq1, Sequencer &seq2) {
  Sequencer &currentActiveSeq = uiState.isVoice2Mode ? seq2 : seq1;
  ParamId paramId = ParamId::Count;

  for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
    if (PARAM_BUTTON_MAPPINGS[i].buttonIndex == buttonIndex) {
      paramId = PARAM_BUTTON_MAPPINGS[i].paramId;
      break;
    }
  }

  if (paramId != ParamId::Count) {
    uint8_t lfoNum = uiState.isVoice2Mode ? 2 : 1;
    currentActiveSeq.assignLFO(lfoNum, paramId);
    Serial.print("Assigned LFO");
    Serial.print(lfoNum);
    Serial.print(" to ");
    Serial.println(CORE_PARAMETERS[static_cast<int>(paramId)].name);
  }
}

static void autoSelectAS5600Parameter(ParamId paramId, UIState &uiState) {
  AS5600ParameterMode newAS5600Param;
  bool isValid = true;
  switch (paramId) {
  case ParamId::Velocity:
    newAS5600Param = AS5600ParameterMode::Velocity;
    break;
  case ParamId::Filter:
    newAS5600Param = AS5600ParameterMode::Filter;
    break;
  case ParamId::Attack:
    newAS5600Param = AS5600ParameterMode::Attack;
    break;
  case ParamId::Decay:
    newAS5600Param = AS5600ParameterMode::Decay;
    break;
  default:
    isValid = false;
    break;
  }

  if (isValid && newAS5600Param != uiState.currentAS5600Parameter) {
    uiState.currentAS5600Parameter = newAS5600Param;
    Serial.print("AS5600 auto-selected: ");
    Serial.println(CORE_PARAMETERS[static_cast<int>(paramId)].name);
  }
}

static void handleAS5600ParameterControl(UIState &uiState) {
  uiState.currentAS5600Parameter = static_cast<AS5600ParameterMode>(
      (static_cast<uint8_t>(uiState.currentAS5600Parameter) + 1) %
      static_cast<uint8_t>(AS5600ParameterMode::COUNT));

  uiState.lastAS5600ButtonPress = millis();

  Serial.print("AS5600 parameter switched to: ");
  switch (uiState.currentAS5600Parameter) {
  case AS5600ParameterMode::Velocity:
    Serial.println("Velocity");
    break;
  case AS5600ParameterMode::Filter:
    Serial.println("Filter");
    break;
  case AS5600ParameterMode::Attack:
    Serial.println("Attack");
    break;
  case AS5600ParameterMode::Decay:
    Serial.println("Decay");
    break;
  case AS5600ParameterMode::DelayTime:
    Serial.println("Delay Time");
    break;
  case AS5600ParameterMode::DelayFeedback:
    Serial.println("Delay Feedback");
    break;
  case AS5600ParameterMode::LFO1freq:
    Serial.println("LFO1 Frequency");
    break;
  case AS5600ParameterMode::LFO1amp:
    Serial.println("LFO1 Amplitude");
    break;
  case AS5600ParameterMode::LFO2freq:
    Serial.println("LFO2 Frequency");
    break;
  case AS5600ParameterMode::LFO2amp:
    Serial.println("LFO2 Amplitude");
    break;
  case AS5600ParameterMode::COUNT:
    break; // Should not happen
  }
}
void pollUIHeldButtons(UIState &uiState, Sequencer &seq1, Sequencer &seq2) {
  unsigned long currentTime = millis();

  // Check for Randomize 1 long press
  if (uiState.randomize1WasPressed && !uiState.randomize1ResetTriggered) {
    if (isLongPress(currentTime - uiState.randomize1PressTime)) {
      seq1.resetAllSteps();
      uiState.resetStepsLightsFlag = true;
      uiState.randomize1ResetTriggered = true;
      Serial.println("Seq 1 reset by long press");
    }
  }

  // Check for Randomize 2 long press
  if (uiState.randomize2WasPressed && !uiState.randomize2ResetTriggered) {
    if (isLongPress(currentTime - uiState.randomize2PressTime)) {
      seq2.resetAllSteps();
      uiState.resetStepsLightsFlag = true;
      uiState.randomize2ResetTriggered = true;
      Serial.println("Seq 2 reset by long press");
    }
  }
}
