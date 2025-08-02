#ifndef UI_STATE_H
#define UI_STATE_H

#include <Arduino.h>
#include "../sequencer/SequencerDefs.h"
#include "../sensors/as5600.h" // For AS5600ParameterMode

/**
 * @brief Centralized state management for the PicoMudrasSequencer UI.
 * 
 * This struct encapsulates all UI-related state variables, eliminating
 * global externs and improving modularity. An instance of this struct
 * is passed to UI functions, making data flow explicit and easier to manage.
 */
struct UIState {
    // --- Parameter Button States ---
    // Indexed by ParamId for direct lookup.
    bool parameterButtonHeld[PARAM_ID_COUNT] = {false};

    // --- Mode States ---
    bool lfoAssignMode = false;
    bool delayOn = true;
    bool modGateParamSeqLengthsMode = false;
    bool slideMode = false;
    bool isVoice2Mode = false;
    int selectedStepForEdit = -1;
    int currentThemeIndex = 0;
    AS5600ParameterMode currentAS5600Parameter = AS5600ParameterMode::Velocity;

    // --- Timing States ---
    unsigned long padPressTimestamps[SEQUENCER_MAX_STEPS] = {0};
    volatile unsigned long flash23Until = 0;
    volatile unsigned long flash25Until = 0;
    volatile unsigned long flash31Until = 0;
    unsigned long lastAS5600ButtonPress = 0;
    unsigned long button24PressTime = 0;
    bool button24WasPressed = false;

    // --- Randomize Button States ---
    unsigned long randomize1PressTime = 0;
    bool randomize1WasPressed = false;
    bool randomize1ResetTriggered = false;
    unsigned long randomize2PressTime = 0;
    bool randomize2WasPressed = false;
    bool randomize2ResetTriggered = false;

    // --- Shuffle State ---
    uint8_t currentShufflePatternIndex = 0;

    // --- Flags ---
    // Flag to signal the LED matrix to reset step lights.
    bool resetStepsLightsFlag = false;

   // --- Debounce for Slide Mode Toggle ---
   unsigned long lastSlideModeToggleTime = 0;

   // --- Settings Mode State ---
   bool settingsMode = false;
   uint8_t settingsMenuIndex = 0;      // 0-7 for 8 menu items
   uint8_t settingsSubMenuIndex = 0;   // For preset selection
   bool inPresetSelection = false;
   uint8_t voice1PresetIndex = 3;      // Default to Lead Voice
   uint8_t voice2PresetIndex = 2;      // Default to Bass Voice
   unsigned long playStopPressTime = 0;
   bool playStopWasPressed = false;
};

#endif // UI_STATE_H