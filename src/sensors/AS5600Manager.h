#ifndef AS5600_MANAGER_H
#define AS5600_MANAGER_H

#include "as5600.h"
#include "../sequencer/SequencerDefs.h"
#include "../dsp/oscillator.h"
#include "../ui/UIState.h"

// Forward declarations
class AS5600Sensor;
struct VoiceState;
namespace daisysp {
    class Oscillator;
}

// Dynamic Flash Speed System for AS5600 boundary proximity feedback
enum class FlashSpeedZone : uint8_t
{
    Normal = 0,  // ±0.0 to ±0.3 range
    Warning = 1, // ±0.3 to ±0.4 range
    Critical = 2 // ±0.4 to ±0.5 range
};

struct FlashSpeedConfig
{
    float speedMultiplier;
    float thresholdStart; // Proximity factor where this zone starts
    float thresholdEnd;   // Proximity factor where this zone ends
};

// Flash speed zones for dynamic boundary proximity feedback
extern const FlashSpeedConfig FLASH_SPEED_ZONES[];
// Helper function to apply increment with boundary checking
void applyIncrementToParameter(AS5600BaseValues* baseValues, AS5600ParameterMode param, float increment);

void updateAS5600BaseValues(UIState& uiState);
void applyAS5600BaseValues( VoiceState *voiceState, const UIState& uiState);
void applyAS5600DelayValues();
void applyAS5600LFOValues();
float getParameterMinValue(AS5600ParameterMode param);
float getParameterMaxValue(AS5600ParameterMode param);
float getAS5600BaseValueRange(AS5600ParameterMode param);
float clampAS5600BaseValue(AS5600ParameterMode param, float value);

// Additional AS5600 helper functions moved from main file
float calculateAS5600BoundaryProximity(AS5600ParameterMode param);
float calculateDynamicFlashSpeed(AS5600ParameterMode param);
void resetAS5600BaseValues(UIState& uiState, bool currentVoiceOnly = true);
void initAS5600BaseValues();

extern AS5600Sensor as5600Sensor;

// Define a new structure for global LFO parameters
struct GlobalLFOParams {
    float lfo1freq;
    float lfo1amp;
    float lfo2freq;
    float lfo2amp;
};

extern GlobalLFOParams globalLFOs;

extern AS5600BaseValuesVoice1 as5600BaseValuesVoice1;
extern AS5600BaseValuesVoice1 as5600BaseValuesVoice2;
extern float delayTarget;
extern float feedbackAmmount;
extern daisysp::Oscillator lfo1;
extern daisysp::Oscillator lfo2;
extern const size_t MAX_DELAY_SAMPLES;


#endif // AS5600_MANAGER_H
