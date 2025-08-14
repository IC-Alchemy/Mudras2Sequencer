#include "AS5600Manager.h"
#include <Arduino.h>
#include "../sequencer/SequencerDefs.h"
#include "../sequencer/Sequencer.h"
#include "../ui/UIState.h"
#include "as5600.h"
#include <algorithm>
#include "../dsp/oscillator.h"

// =======================
//   EXTERNAL REFERENCES
// =======================

// Reference to the global UIState from main file
extern UIState uiState;

// =======================
//   AS5600 GLOBAL VARIABLES
// =======================

// AS5600 global variables moved from main file
// Note: currentAS5600Parameter is now accessed via uiState.currentAS5600Parameter
AS5600BaseValuesVoice1 as5600BaseValuesVoice1;
AS5600BaseValuesVoice1 as5600BaseValuesVoice2;
unsigned long lastAS5600ButtonPress = 0;

// Flash speed zones for dynamic boundary proximity feedback
const FlashSpeedConfig FLASH_SPEED_ZONES[] = {
    {1.0f, 0.0f, 0.65f},    // Normal: 1x speed,
    {2.0f, 0.65f, 0.8375f}, // Warning: 2x speed,
    {3.0f, 0.8375f, 1.0f}   // Critical: 3x speed,
};

// =======================
//   AS5600 PARAMETER BOUNDS MANAGEMENT
// =======================

float getParameterMinValue(AS5600ParameterMode param)
{
    // Return the minimum valid value for each parameter type
    switch (param)
    {
    case AS5600ParameterMode::Velocity:
    case AS5600ParameterMode::Filter:
    case AS5600ParameterMode::Attack:
    case AS5600ParameterMode::Decay:
        return 0.0f;
    case AS5600ParameterMode::Note:
        return 0.0f; // Note range: 0.0 to 21.0 (scale array indices)
    case AS5600ParameterMode::DelayTime:
        return 120.0f; // 10ms minimum delay (480 samples at 48kHz)
    case AS5600ParameterMode::DelayFeedback:
        return 0.0f;
    case AS5600ParameterMode::SlideTime:
        return 0.0f; // Minimum slide time (instant)

    default:
        return 0.0f;
    }
}

float getParameterMaxValue(AS5600ParameterMode param)
{
    // Return the maximum valid value for each parameter type
    switch (param)
    {
    case AS5600ParameterMode::Velocity:
    case AS5600ParameterMode::Filter:
    case AS5600ParameterMode::Attack:
    case AS5600ParameterMode::Decay:
        return 1.0f;
    case AS5600ParameterMode::Note:
        return 21.0f; // Note range: 0.0 to 21.0 (scale array indices)
    case AS5600ParameterMode::DelayTime:
        return MAX_DELAY_SAMPLES * .85f; // Maximum delay in samples (1.8 seconds)
    case AS5600ParameterMode::DelayFeedback:
        return 0.91f; // Maximum 95% feedback to prevent excessive feedback
    case AS5600ParameterMode::SlideTime:
        return 1.0f; // Maximum slide time

    default:
        return 1.0f;
    }
}

float getAS5600BaseValueRange(AS5600ParameterMode param)
{
    // Return the allowed range for AS5600 base values
    float fullRange = getParameterMaxValue(param) - getParameterMinValue(param);

    // Delay parameters use full range without restrictions
    if (param == AS5600ParameterMode::DelayTime || param == AS5600ParameterMode::DelayFeedback)
   
    {
        return fullRange; // Full range for delay parameters
    }

    // SlideTime uses full range without restrictions
    if (param == AS5600ParameterMode::SlideTime)
    {
        return fullRange; // Full range for slide time parameter
    }

    // Other parameters use reduced range to leave room for sequencer values
    return fullRange * 0.75f; // 60% of full range for sequencer parameters
}

float clampAS5600BaseValue(AS5600ParameterMode param, float value)
{
    // Clamp AS5600 base values to their allowed bidirectional range
    float maxRange = getAS5600BaseValueRange(param);
    return std::max(-maxRange, std::min(value, maxRange));
}
// --- Update AS5600 Base Values (Bidirectional Velocity-Sensitive Control) ---
void updateAS5600BaseValues(UIState& uiState)
{
    if (!as5600Sensor.isConnected())
    {
        return;
    }

    // Check if we're in edit mode for a specific step
    if (uiState.selectedStepForEdit >= 0)
    {
        updateAS5600StepParameterValues(uiState);
        return;
    }

    // Get current AS5600 base values for the active voice
    AS5600BaseValues *activeBaseValues = uiState.isVoice2Mode ? (AS5600BaseValues *)&as5600BaseValuesVoice2 : (AS5600BaseValues *)&as5600BaseValuesVoice1;

    // Get bidirectional velocity-sensitive parameter increment
    float minVal = getParameterMinValue(uiState.currentAS5600Parameter);
    float maxVal = getParameterMaxValue(uiState.currentAS5600Parameter);
    float increment = as5600Sensor.getParameterIncrement(minVal - maxVal, maxVal - minVal, 3);

    // Ignore tiny increments to prevent noise from affecting parameters
    const float MINIMUM_INCREMENT_THRESHOLD = 0.0005f;
    if (abs(increment) < MINIMUM_INCREMENT_THRESHOLD)
    {
        return;
    }

    // Apply increment to the appropriate parameter with boundary checking
    applyIncrementToParameter(activeBaseValues, uiState.currentAS5600Parameter, increment);
}

// --- Update AS5600 Step Parameter Values (Edit Mode) ---
void updateAS5600StepParameterValues(UIState& uiState)
{
    if (!as5600Sensor.isConnected() || uiState.selectedStepForEdit < 0 || uiState.currentEditParameter == ParamId::Count)
    {
        return;
    }

    // Get the active sequencer based on selected voice (1..4)
    extern Sequencer seq1, seq2, seq3, seq4;
    Sequencer& activeSeq = (uiState.selectedVoiceIndex == 0) ? seq1 :
                           (uiState.selectedVoiceIndex == 1) ? seq2 :
                           (uiState.selectedVoiceIndex == 2) ? seq3 : seq4;

    // Use the currently selected edit parameter
    ParamId targetParamId = uiState.currentEditParameter;
    if (targetParamId == ParamId::Count)
    {
        return; // No parameter selected for editing
    }

    // Get parameter range for the target parameter
    float minVal = getParameterMinValueForParamId(targetParamId);
    float maxVal = getParameterMaxValueForParamId(targetParamId);
    
    // Get velocity-sensitive increment with full range scaling
    float increment = as5600Sensor.getParameterIncrement(minVal - maxVal, maxVal - minVal, 3);

    // Ignore tiny increments to prevent noise
    const float MINIMUM_INCREMENT_THRESHOLD = 0.0005f;
    if (abs(increment) < MINIMUM_INCREMENT_THRESHOLD)
    {
        return;
    }

    // Get current parameter value for the selected step
    uint8_t stepIndex = static_cast<uint8_t>(uiState.selectedStepForEdit);
    float currentValue = activeSeq.getStepParameterValue(targetParamId, stepIndex);
    
    // Apply increment with boundary checking
    float newValue = currentValue + increment;
    newValue = std::max(minVal, std::min(newValue, maxVal));
    
    // Set the new parameter value
    activeSeq.setStepParameterValue(targetParamId, stepIndex, newValue);
    
    // Trigger immediate OLED update by updating the active voice state
    extern void updateActiveVoiceState(uint8_t stepIndex, Sequencer& activeSeq);
    updateActiveVoiceState(stepIndex, activeSeq);
    
    // Debug output
    Serial.print("AS5600 Edit Mode - Step ");
    Serial.print(stepIndex);
    Serial.print(", Parameter: ");
    Serial.print(CORE_PARAMETERS[static_cast<int>(targetParamId)].name);
    Serial.print(", Value: ");
    Serial.print(newValue, 3);
    Serial.print(" (");
    Serial.print(formatParameterValueForDisplay(targetParamId, newValue));
    Serial.println(")");
}

// Helper function to apply increment with boundary checking
void applyIncrementToParameter(AS5600BaseValues *baseValues, AS5600ParameterMode param, float increment)
{
    float *targetValue = nullptr;

    // Select the appropriate parameter to modify
    switch (param)
    {
    case AS5600ParameterMode::Velocity:
        targetValue = &baseValues->velocity;
        break;
    case AS5600ParameterMode::Filter:
        targetValue = &baseValues->filter;
        break;
    case AS5600ParameterMode::Attack:
        targetValue = &baseValues->attack;
        break;
    case AS5600ParameterMode::Decay:
        targetValue = &baseValues->decay;
        break;
    case AS5600ParameterMode::DelayTime:
        targetValue = &baseValues->delayTime;
        break;
    case AS5600ParameterMode::DelayFeedback:
        targetValue = &baseValues->delayFeedback;
        break;
    case AS5600ParameterMode::SlideTime:
        targetValue = &baseValues->slideTime;
        break;

    default:
        return; // Invalid parameter
    }

    if (!targetValue)
    {
        return; // Safety check
    }

    // Apply increment with boundary checking
    float newValue = *targetValue + increment;
    float oldValue = *targetValue;

    // For bidirectional parameters (voice parameters)
    if (param <= AS5600ParameterMode::Decay)
    {
        float maxRange = getAS5600BaseValueRange(param);
        *targetValue = std::max(-maxRange, std::min(newValue, maxRange));
    }
    // For unidirectional parameters (delay and slide time)
    else
    {
        float minVal = getParameterMinValue(param);
        float maxVal = getParameterMaxValue(param);
        *targetValue = std::max(minVal, std::min(newValue, maxVal));
    }



    // Debug output for delay parameter changes (uncomment for debugging)
    /*
    if (param == AS5600ParameterMode::DelayTime || param == AS5600ParameterMode::DelayFeedback)
    {
        Serial.print("AS5600 ");
        Serial.print(param == AS5600ParameterMode::DelayTime ? "DelayTime" : "DelayFeedback");
        Serial.print(" changed from ");
        Serial.print(oldValue, 3);
        Serial.print(" to ");
        Serial.print(*targetValue, 3);
        Serial.print(" (increment: ");
        Serial.print(increment, 3);
        Serial.println(")");
    }
    */
}

// --- Helper Functions for Step Parameter Editing ---

// Convert AS5600ParameterMode to ParamId for step editing
ParamId convertAS5600ParameterToParamId(AS5600ParameterMode as5600Param)
{
    switch (as5600Param)
    {
    case AS5600ParameterMode::Velocity:
        return ParamId::Velocity;
    case AS5600ParameterMode::Filter:
        return ParamId::Filter;
    case AS5600ParameterMode::Attack:
        return ParamId::Attack;
    case AS5600ParameterMode::Decay:
        return ParamId::Decay;
    case AS5600ParameterMode::Note:
        return ParamId::Note;
    case AS5600ParameterMode::SlideTime:
        return ParamId::Count; // SlideTime is not a step parameter
    default:
        return ParamId::Count; // Invalid for step editing
    }
}

// Get parameter minimum value for ParamId
float getParameterMinValueForParamId(ParamId paramId)
{
    switch (paramId)
    {
    case ParamId::Velocity:
    case ParamId::Filter:
    case ParamId::Attack:
    case ParamId::Decay:
        return 0.0f;
    case ParamId::Note:
        return 0.0f; // Note range: 0.0 to 21.0 (scale array indices)
    default:
        return 0.0f;
    }
}

// Get parameter maximum value for ParamId
float getParameterMaxValueForParamId(ParamId paramId)
{
    switch (paramId)
    {
    case ParamId::Velocity:
    case ParamId::Filter:
    case ParamId::Attack:
    case ParamId::Decay:
        return 1.0f;
    case ParamId::Note:
        return 21.0f; // Note range: 0.0 to 21.0 (scale array indices)
    default:
        return 1.0f;
    }
}

// Format parameter value for display (similar to OLED formatParameterValue)
String formatParameterValueForDisplay(ParamId paramId, float value)
{
    switch (paramId)
    {
    case ParamId::Note:
        return String((int)value);
    case ParamId::Velocity:
        return String((int)(value * 100)) + "%";
    case ParamId::Filter:
    {
        int filterFreq = daisysp::fmap(value, 100.0f, 9710.0f, daisysp::Mapping::EXP);
        return String(filterFreq) + "Hz";
    }
    case ParamId::Attack:
    case ParamId::Decay:
        return String(value, 3) + "s";
    default:
        return String(value, 2);
    }
}

// Helper function for the "Shift and Scale" mapping.
// This function takes a sequencer value (0.0-1.0) and an AS5600 offset
// (a bipolar value, e.g., -0.6 to 0.6) and combines them intelligently.
float shiftAndScale(float seqValue, float as5600Offset)
{
    float finalValue;
    if (as5600Offset >= 0.0f)
    {
        // When the AS5600 offset is positive, it sets the minimum value,
        // and the sequencer value is scaled to fit the remaining range up to 1.0.
        finalValue = as5600Offset + (seqValue * (1.0f - as5600Offset));
    }
    else
    {
        // When the AS5600 offset is negative, it reduces the maximum value,
        // and the sequencer value is scaled to fit the range from 0.0 up to that new maximum.
        finalValue = seqValue * (1.0f + as5600Offset);
    }
    // Clamp the result to ensure it remains within the valid [0.0, 1.0] range.
    return std::max(0.0f, std::min(finalValue, 1.0f));
}

/**
 * Apply AS5600 magnetic encoder base values to voice parameters.
 * Implements a "Shift and Scale" mapping to combine encoder and sequencer values.
 * This avoids "dead zones" by scaling the sequencer's output within the range
 * defined by the encoder's offset.
 * */
void applyAS5600BaseValues(VoiceState *voiceState, uint8_t voiceId)
{
    if (!as5600Sensor.isConnected() || !voiceState)
    {
        return;
    }

    // Select the correct base values based on voice ID (0 = voice1, 1 = voice2)
    const AS5600BaseValues *baseValues = (voiceId == 1) ? (const AS5600BaseValues *)&as5600BaseValuesVoice2 : (const AS5600BaseValues *)&as5600BaseValuesVoice1;

    // Apply "Shift and Scale" for each parameter.
    // This maps the sequencer value into the dynamic range set by the AS5600 offset.
    voiceState->velocity = shiftAndScale(voiceState->velocity, baseValues->velocity);
    voiceState->filter = shiftAndScale(voiceState->filter, baseValues->filter);
    voiceState->attack = shiftAndScale(voiceState->attack, baseValues->attack);
    voiceState->decay = shiftAndScale(voiceState->decay, baseValues->decay);
}

/**
 * Apply AS5600 magnetic encoder values to global delay effect parameters.
 * Direct parameter control: delay parameters use full range without restrictions.
 * Thread-safe communication for Core0 audio processing.
 */
void applyAS5600DelayValues()
{
    if (!as5600Sensor.isConnected())
    {
        return;
    }

    // Use Voice 1 base values for global delay parameters (delay is not per-voice)
    const AS5600BaseValuesVoice1 *baseValues = &as5600BaseValuesVoice1;

    // Apply delay time directly (already clamped to 10ms-1.8s range in updateAS5600BaseValues)
    delayTarget = baseValues->delayTime;

    // Apply delay feedback directly (already clamped to 0.0-1.0 range in updateAS5600BaseValues)
    feedbackAmmount = baseValues->delayFeedback;
}
 // ----------------------
// Helper: Update slide time on the active voice
// ----------------------
 void updateAS5600SlideTime(uint8_t voiceId, float slideTime)
{
      if (!as5600Sensor.isConnected()|| !uiState.slideMode)
    {
        return;
    }
        const AS5600BaseValues *activeBaseValues = uiState.isVoice2Mode ? &as5600BaseValuesVoice2 : &as5600BaseValuesVoice1;
//  finish up by updating the base values
    applyIncrementToParameter((AS5600BaseValues *)activeBaseValues, AS5600ParameterMode::SlideTime, slideTime);


}



// =======================
//   AS5600 HELPER FUNCTIONS (moved from main file)
// =======================

/**
 * Gets the current value of the active AS5600 parameter, normalized to a 0.0-1.0 range.
 * This is used for visual feedback, such as controlling the brightness or color of an LED.
 */
float getAS5600ParameterValue()
{
    if (!as5600Sensor.isConnected())
    {
        return 0.0f;
    }

    const AS5600BaseValues *activeBaseValues = uiState.isVoice2Mode ? &as5600BaseValuesVoice2 : &as5600BaseValuesVoice1;
    float value = 0.0f;

    // Retrieve the raw value for the current parameter
    switch (uiState.currentAS5600Parameter)
    {
    case AS5600ParameterMode::Velocity:
        value = activeBaseValues->velocity;
        break;
    case AS5600ParameterMode::Filter:
        value = activeBaseValues->filter;
        break;
    case AS5600ParameterMode::Attack:
        value = activeBaseValues->attack;
        break;
    case AS5600ParameterMode::Decay:
        value = activeBaseValues->decay;
        break;
    case AS5600ParameterMode::DelayTime:
        value = activeBaseValues->delayTime;
        break;
    case AS5600ParameterMode::DelayFeedback:
        value = activeBaseValues->delayFeedback;
        break;
    case AS5600ParameterMode::SlideTime:
        value = activeBaseValues->slideTime;
        break;
   
    }

    // Normalize the value to a 0.0-1.0 range for LED feedback
    float minVal = getParameterMinValue(uiState.currentAS5600Parameter);
    float maxVal = getParameterMaxValue(uiState.currentAS5600Parameter);
    float normalizedValue = (value - minVal) / (maxVal - minVal);

    // For bipolar parameters (like velocity, filter, etc.), we need to handle the normalization differently.
    // Since they range from -maxRange to +maxRange, we can map this to 0.0-1.0.
    if (uiState.currentAS5600Parameter <= AS5600ParameterMode::Decay) // Assuming these are the bipolar params
    {
        float maxRange = getAS5600BaseValueRange(uiState.currentAS5600Parameter);
        normalizedValue = (value + maxRange) / (2 * maxRange);
    }

    return std::max(0.0f, std::min(normalizedValue, 1.0f)); // Clamp to ensure valid range
}

/**
 * Initialize AS5600 base values with proper defaults
 */
void initAS5600BaseValues()
{
    // Initialize AS5600 base values with proper defaults
    // Voice parameters start at neutral (0.0f)
    as5600BaseValuesVoice1.velocity = 0.0f;
    as5600BaseValuesVoice1.filter = 0.0f;
    as5600BaseValuesVoice1.attack = 0.0f;
    as5600BaseValuesVoice1.decay = 0.0f;

    as5600BaseValuesVoice2.velocity = 0.0f;
    as5600BaseValuesVoice2.filter = 0.0f;
    as5600BaseValuesVoice2.attack = 0.0f;
    as5600BaseValuesVoice2.decay = 0.0f;

    // Delay parameters start with reasonable defaults (full range values)
    // Reset delay parameters to defaults (shared between voices)
    as5600BaseValuesVoice1.delayTime = 48000.0f * 0.2f; // 200ms default delay
    as5600BaseValuesVoice1.delayFeedback = 0.55f;       // 55% default feedback
    as5600BaseValuesVoice2.delayTime = 48000.0f * 0.2f; // 200ms default delay for voice2
    as5600BaseValuesVoice2.delayFeedback = 0.55f;       // 55% default feedback for voice2

}

/**
 * Reset AS5600 base values to defaults
 * @param uiState Reference to the UI state for voice mode information
 * @param currentVoiceOnly If true, only reset the currently active voice
 */
void resetAS5600BaseValues(UIState& uiState, bool currentVoiceOnly)
{
    if (currentVoiceOnly)
    {
        // Reset only the currently active voice
        AS5600BaseValues *activeBaseValues = uiState.isVoice2Mode ? (AS5600BaseValues *)&as5600BaseValuesVoice2 : (AS5600BaseValues *)&as5600BaseValuesVoice1;

        // Reset voice parameters to neutral
        activeBaseValues->velocity = 0.0f;
        activeBaseValues->filter = 0.0f;
        activeBaseValues->attack = 0.0f;
        activeBaseValues->decay = 0.0f;

        // Delay parameters are global and not reset with this function
    }
    else
    {
        // Reset all voices - call the full initialization
        initAS5600BaseValues();
    }
}