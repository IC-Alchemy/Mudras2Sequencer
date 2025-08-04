#include "ButtonManager.h"

// =======================
//   BUTTON MAPPING ARRAY
// =======================

// Defines the mapping from physical button indices to parameter IDs and names.
const ParamButtonMapping PARAM_BUTTON_MAPPINGS[] = {
    {16, ParamId::Note,     "Note"},
    {17, ParamId::Velocity, "Velocity"},
    {18, ParamId::Filter,   "Filter"},
    {19, ParamId::Attack,   "Attack"},
    {20, ParamId::Decay,    "Decay"},
    {21, ParamId::Octave,   "Octave"},
    {22, ParamId::Slide,    "Slide"}
};

const size_t PARAM_BUTTON_MAPPINGS_SIZE = sizeof(PARAM_BUTTON_MAPPINGS) / sizeof(PARAM_BUTTON_MAPPINGS[0]);

// =======================
//   FUNCTION IMPLEMENTATIONS
// =======================

void initButtonManager(UIState& uiState) {
    // Reset all button-related states to their defaults.
    for (int i = 0; i < PARAM_ID_COUNT; ++i) {
        uiState.parameterButtonHeld[i] = false;
    }
    for (int i = 0; i < SEQUENCER_MAX_STEPS; ++i) {
        uiState.padPressTimestamps[i] = 0;
    }
    
    uiState.delayOn = true;
    uiState.modGateParamSeqLengthsMode = false;
    uiState.slideMode = false;
    uiState.isVoice2Mode = false;
    uiState.selectedStepForEdit = -1;
    uiState.flash23Until = 0;
    uiState.flash25Until = 0;
    uiState.flash31Until = 0;
    uiState.lastAS5600ButtonPress = 0;
    uiState.button24PressTime = 0;
    uiState.button24WasPressed = false;
    uiState.resetStepsLightsFlag = false;
}

bool isLongPress(unsigned long pressDuration) {
    return pressDuration >= LONG_PRESS_THRESHOLD;
}

bool isAnyParameterButtonHeld(const UIState& uiState) {
    for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
        ParamId paramId = PARAM_BUTTON_MAPPINGS[i].paramId;
        // Skip Slide param if in slide mode
        if (paramId == ParamId::Slide && uiState.slideMode) {
            continue;
        }
        if (uiState.parameterButtonHeld[static_cast<int>(paramId)]) {
            return true;
        }
    }
    return false;
}

const ParamButtonMapping* getHeldParameterButton(const UIState& uiState) {
    for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
        const auto& mapping = PARAM_BUTTON_MAPPINGS[i];
        // Skip Slide param if in slide mode
        if (mapping.paramId == ParamId::Slide && uiState.slideMode) {
            continue;
        }
        if (uiState.parameterButtonHeld[static_cast<int>(mapping.paramId)]) {
            return &mapping;
        }
    }
    return nullptr;
}

