#ifndef SEQUENCER_DEFS_H
#define SEQUENCER_DEFS_H

#include <stdint.h>
#include <variant> // Required for std::variant
constexpr uint16_t PULSES_PER_QUARTER_NOTE = 480;
constexpr uint8_t PULSES_PER_SEQUENCER_STEP = PULSES_PER_QUARTER_NOTE / 4;
constexpr uint8_t SEQUENCER_MAX_STEPS = 64;    // Max steps in sequencer
constexpr uint8_t MIN_STEPS = 2;
constexpr uint8_t DEFAULT_STEPS = 16;

enum class ParamId : uint8_t
{ // Define parameter IDs, these must match the order of the CORE_PARAMETERS array
    Note,       // 0
    Velocity,   // 1
    Filter,     // 2
    Attack,     // 4
    Decay,      // 5
    Octave,     // 6
    GateLength, // 7
    Gate,       // 8
    Slide,      // 9
    Count
};

// Constant for array sizing based on ParamId::Count
constexpr uint8_t PARAM_ID_COUNT = static_cast<uint8_t>(ParamId::Count);

// AS5600 parameter cycling system (moved from UIEventHandler.h to break circular dependency)
enum class AS5600ParameterMode : uint8_t {
    Velocity = 0,
    Filter = 1,
    Attack = 2,
    Decay = 3,
    Note = 4,
    DelayTime = 5,
    DelayFeedback = 6,
    SlideTime = 7,
    COUNT = 8
};

// AS5600 parameter base values (per voice) - supports bidirectional control
struct AS5600BaseValues {
    float velocity = 0.0f;
    float filter = 0.0f;
    float attack = 0.0f;
    float decay = 0.0f;
    float delayTime = 0.0f;       // Delay time offset for global delay
    float delayFeedback = 0.0f;   // Delay feedback offset for global delay
    float slideTime = 0.0f;       // Slide time in seconds for voice glide
};

// Voice-specific AS5600 base values (same structure as base, no shadowing)
struct AS5600BaseValuesVoice1 : public AS5600BaseValues {
    // No additional members - delay parameters are inherited from base class
};
// Define StepEditButtons struct for step parameter edit state (6 buttons)
struct StepEditButtons
{
    bool note;
    bool velocity;
    bool filter;
    bool attack;
    bool decay;
    bool octave;
};

// Fixed-size parameter value array
template <uint8_t SIZE>
struct ParameterTrack
{
    float values[SIZE];
    uint8_t stepCount;
    float defaultValue;

    // Initialize track with default values
    void init(float defValue)
    {
        defaultValue = defValue;
        stepCount = DEFAULT_STEPS;
        for (uint8_t i = 0; i < SIZE; ++i)
        {
            values[i] = defValue;
        }
    }

    // Get value for any step index (handles wrapping)
    float getValue(uint8_t stepIdx) const
    {
        if (stepCount == 0)
        {
            return defaultValue; // Prevent division by zero
        }
        return values[stepIdx % stepCount];
    }

    // Set value for a specific step (handles wrapping)
    void setValue(uint8_t stepIdx, float value)
    {
        if (stepCount == 0)
        {
            return; // Prevent division by zero
        }
        values[stepIdx % stepCount] = value;
    }

    // Resize track to new step count
    void resize(uint8_t newStepCount)
    {
        if (newStepCount >= MIN_STEPS && newStepCount <= SIZE)
        {
            // Preserve existing values
            if (newStepCount > stepCount)
            {
                for (uint8_t i = stepCount; i < newStepCount; ++i)
                {
                    values[i] = defaultValue;
                }
            }
            stepCount = newStepCount;
        }
    }
};

// Define the variant type for parameter values that can be int, float, or bool
using ParameterValueType = std::variant<int, float, bool>;

// Parameter definition with metadata
struct ParameterDefinition
{
    const char *name;                // Display name
    ParameterValueType defaultValue; // Default parameter value
    ParameterValueType minValue;     // Minimum allowed value
    ParameterValueType maxValue;     // Maximum allowed value
    bool isBinary;                   // True for gate/slide-like parameters
    uint8_t defaultSteps;            // Default number of steps
};

// Core parameter definitions, must match the order of the ParamId enum
//  "paramID", "defaultValue", "minValue", "maxValue", "isBinary", "defaultSteps"
constexpr ParameterDefinition CORE_PARAMETERS[] = {

    {"Note", 0.f, 0.f, 21.f, false, DEFAULT_STEPS},         // ParamId::Note - Default: float
    {"Velocity", 0.5f, 0.0f, 1.0f, false, DEFAULT_STEPS},   // ParamId::Velocity - Default: float
    {"Filter", 0.5f, 0.0f, 1.0f, false, DEFAULT_STEPS},     // ParamId::Filter - Default: float
    {"Attack", 0.01f, 0.0f, 1.f, false, DEFAULT_STEPS},  // ParamId::Attack - Default: float
    {"Decay", 0.11f, 0.f, 1.0f, false, DEFAULT_STEPS},    // ParamId::Decay - Default: float
    {"Octave", 0.0f, 0.f, 1.0f, false, DEFAULT_STEPS},    // ParamId::Octave - Default: float (interpreted as -1, 0, 1)
    {"GateLength", 0.1f, 0.001f, 1.0f, false, DEFAULT_STEPS}, // ParamId::GateLength - Default: float
    {"Gate", false, false, true, true, DEFAULT_STEPS},      // ParamId::Gate - Default: bool
    {"Slide", false, false, true, true, DEFAULT_STEPS}     // ParamId::Slide - Default: bool
};

// Voice state for sequencer output,  this is the struct that is passed to the audio output
struct VoiceState
{
    float note = 0.0f;      // Raw note value (0-21) for scale array lookup - matches Note parameter range
    float velocity = 0.8f;
    float filter = 0.37f;
    float attack = 0.01f;
    float decay = 0.01f;
    float octave = 0.f;     // Octave offset in semitones (-12, 0, +12)
    uint16_t gateLength = PULSES_PER_SEQUENCER_STEP/2;

    bool gate = true;
    bool slide = false;
    bool retrigger = false; // Add this flag to command the envelope to re-start
};

// Struct representing all step parameters for a sequencer step
// this is the struct that is passed to the sequencer
struct Step
{

    float note = 0.0f;
    float velocity = 0.5f;
    float filter;         // Filter cutoff (0.0 to 1.0)
    float attack = 0.04f;
    float decay = 0.1f;
    float octave = 0.f;                                // Added octave member
    uint16_t gateLength = PULSES_PER_SEQUENCER_STEP/2;;
    bool gate = false;
    bool slide = false;
};

// Gate timing system for automatic gate turn-off
struct GateTimer
{
    volatile bool active = false;
    volatile uint16_t remainingTicks = 0;
    volatile uint32_t totalTicksProcessed = 0; // Debug counter

    void start(uint16_t durationTicks) volatile
    {
        active = true;
        remainingTicks = durationTicks;
        totalTicksProcessed = 0; // Reset debug counter
    }

    void tick() volatile
    {
        totalTicksProcessed++; // Always increment for debugging
        if (active && remainingTicks > 0)
        {
            remainingTicks--;
            if (remainingTicks == 0)
            {
                active = false;
            }
        }
    }

    void stop() volatile
    {
        active = false;
        remainingTicks = 0;
    }

    bool isExpired() const volatile
    {
        return !active && remainingTicks == 0;
    }
};

// --- Utility Functions ---
float mapNormalizedValueToParamRange(ParamId id, float normalizedValue);

#endif // SEQUENCER_DEFS_H
