#include "AS5600Manager.h"
#include <Arduino.h>
#include "../sequencer/SequencerDefs.h"
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
GlobalLFOParams globalLFOs; // Definition of the global LFO parameters
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
    case AS5600ParameterMode::DelayTime:
        return 480.0f; // 10ms minimum delay (480 samples at 48kHz)
    case AS5600ParameterMode::DelayFeedback:
        return 0.0f;
    case AS5600ParameterMode::LFO1freq:
        return 0.0001f;
    case AS5600ParameterMode::LFO2freq:
        return 0.0001f; // 0.1Hz minimum LFO frequency
    case AS5600ParameterMode::LFO1amp:
    case AS5600ParameterMode::LFO2amp:
        return 0.0f; // 0% minimum amplitude
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
    case AS5600ParameterMode::DelayTime:
        return MAX_DELAY_SAMPLES * .75f; // Maximum delay in samples (1.8 seconds)
    case AS5600ParameterMode::DelayFeedback:
        return 0.91f; // Maximum 95% feedback to prevent excessive feedback
    case AS5600ParameterMode::LFO1freq:
    case AS5600ParameterMode::LFO2freq:
        return 8.0f; // 20Hz maximum LFO frequency
    case AS5600ParameterMode::LFO1amp:
    case AS5600ParameterMode::LFO2amp:
        return 1.0f; // 100% maximum amplitude
    default:
        return 1.0f;
    }
}

float getAS5600BaseValueRange(AS5600ParameterMode param)
{
    // Return the allowed range for AS5600 base values
    float fullRange = getParameterMaxValue(param) - getParameterMinValue(param);

    // Delay and LFO parameters use full range without restrictions
    if (param == AS5600ParameterMode::DelayTime || param == AS5600ParameterMode::DelayFeedback ||
        param == AS5600ParameterMode::LFO1freq || param == AS5600ParameterMode::LFO1amp ||
        param == AS5600ParameterMode::LFO2freq || param == AS5600ParameterMode::LFO2amp)
    {
        return fullRange; // Full range for delay and LFO parameters
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
    case AS5600ParameterMode::LFO1freq:
        targetValue = &globalLFOs.lfo1freq;
        break;
    case AS5600ParameterMode::LFO1amp:
        targetValue = &globalLFOs.lfo1amp;
        break;
    case AS5600ParameterMode::LFO2freq:
        targetValue = &globalLFOs.lfo2freq;
        break;
    case AS5600ParameterMode::LFO2amp:
        targetValue = &globalLFOs.lfo2amp;
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
    // For unidirectional parameters (delay and LFO)
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
void applyAS5600BaseValues(VoiceState *voiceState, const UIState& uiState)
{
    if (!as5600Sensor.isConnected() || !voiceState)
    {
        return;
    }

    const AS5600BaseValues *baseValues = uiState.isVoice2Mode ? (const AS5600BaseValues *)&as5600BaseValuesVoice2 : (const AS5600BaseValues *)&as5600BaseValuesVoice1;

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

    // Debug output for delay parameter updates (uncomment for debugging)
    /*
    Serial.print("Delay Update - DelayTime: ");
    Serial.print(baseValues->delayTime, 1);
    Serial.print(" samples, DelayFeedback: ");
    Serial.println(baseValues->delayFeedback, 3);
    */
}

/**
 * Apply AS5600 magnetic encoder values to global LFO parameters.
 * Direct parameter control: LFO parameters use full range without restrictions.
 * Thread-safe communication for Core0 audio processing.
 */
void applyAS5600LFOValues()
{
    if (!as5600Sensor.isConnected())
    {
        return;
    }

    // Apply LFO frequencies directly from the global LFO struct
    lfo1.SetFreq(globalLFOs.lfo1freq);
    lfo2.SetFreq(globalLFOs.lfo2freq);

    // Debug output for LFO parameter updates (uncomment for debugging)
/*
    Serial.print("LFO Update - LFO1freq: ");
    Serial.print(globalLFOs.lfo1freq, 2);
    Serial.print("Hz, LFO1amp: ");
    Serial.print(globalLFOs.lfo1amp, 3);
    Serial.print(", LFO2freq: ");
    Serial.print(globalLFOs.lfo2freq, 2);
    Serial.print("Hz, LFO2amp: ");
    Serial.println(globalLFOs.lfo2amp, 3);
    */
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
    case AS5600ParameterMode::LFO1freq:
        value = globalLFOs.lfo1freq;
        break;
    case AS5600ParameterMode::LFO1amp:
        value = globalLFOs.lfo1amp;
        break;
    case AS5600ParameterMode::LFO2freq:
        value = globalLFOs.lfo2freq;
        break;
    case AS5600ParameterMode::LFO2amp:
        value = globalLFOs.lfo2amp;
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

    // LFO parameters start with reasonable defaults
    globalLFOs.lfo1freq = .01f;   // 1Hz default
    globalLFOs.lfo1amp = 0.0f;  // No modulation initially
    globalLFOs.lfo2freq = .01f; // 2Hz default
    globalLFOs.lfo2amp = 0.0f;  // No modulation initially
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

        // LFO parameters are global and not reset with this function
    }
    else
    {
        // Reset all voices - call the full initialization
        initAS5600BaseValues();
    }
}