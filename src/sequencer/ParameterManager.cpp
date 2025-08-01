#include "ParameterManager.h"
#include <algorithm> // For std::max, std::min
#include <cmath>     // For roundf
#include <random>    // For std::default_random_engine, std::uniform_real_distribution
#include <chrono>    // For std::chrono::system_clock (for seeding)
#include <variant>   // For std::visit
#include "../sensors/as5600.h" // For AS5600ParameterMode
#include "../sensors/AS5600Manager.h" // For MAX_DELAY_SAMPLES extern declaration

// AS5600 parameter bounds management functions moved to src/sensors/AS5600Manager.cpp

// Helper function to safely get float from ParameterValueType variant
// This function is internal to ParameterManager.cpp
static float getFloatFromParameterValueType(const ParameterValueType &v)
{
    return std::visit([](auto &&arg) -> float
                      {
                          using T = std::decay_t<decltype(arg)>;
                          if constexpr (std::is_same_v<T, float>)
                              return arg;
                          if constexpr (std::is_same_v<T, int>)
                              return static_cast<float>(arg);
                          if constexpr (std::is_same_v<T, bool>)
                              return arg ? 1.0f : 0.0f;
                          return 0.0f; // Fallback for unexpected types
                      },
                      v);
}

ParameterManager::ParameterManager()
{
    // Initialize the spin lock in the constructor
    _lock = spin_lock_init(spin_lock_claim_unused(true)); // Claim a unique lock number
}

void ParameterManager::init()
{
    for (size_t i = 0; i < static_cast<size_t>(ParamId::Count); ++i)
    {
        // Initialize each track with its default value from CORE_PARAMETERS
        _tracks[i].init(getFloatFromParameterValueType(CORE_PARAMETERS[i].defaultValue));
    }
}

void ParameterManager::setStepCount(ParamId id, uint8_t steps)
{
    _tracks[static_cast<size_t>(id)].resize(steps);
}

uint8_t ParameterManager::getStepCount(ParamId id) const
{
    uint8_t count = _tracks[static_cast<size_t>(id)].stepCount;
    return count;
}

float ParameterManager::getValue(ParamId id, uint8_t stepIdx) const
{
    float value = _tracks[static_cast<size_t>(id)].getValue(stepIdx);
    return value;
}

void ParameterManager::setValue(ParamId id, uint8_t stepIdx, float value)
{

    // Apply clamping and rounding based on parameter definition
    const auto &paramDef = CORE_PARAMETERS[static_cast<size_t>(id)];
    float minVal = getFloatFromParameterValueType(paramDef.minValue);
    float maxVal = getFloatFromParameterValueType(paramDef.maxValue);

    float clampedValue = std::max(minVal, std::min(value, maxVal));

    if (paramDef.isBinary)
    { // For boolean parameters, round to 0 or 1
        clampedValue = (clampedValue > 0.5f) ? 1.0f : 0.0f;
    }
    else if (paramDef.minValue.index() == 0)
    { // If min value is int, assume integer parameter
        clampedValue = roundf(clampedValue);
    }

    // DEBUG: Trace parameter setting (only for Note parameter to reduce spam)
    /*
    if (id == ParamId::Note) {
        Serial.print("[PARAM SET DEBUG] Note step ");
        Serial.print(stepIdx);
        Serial.print(": ");
        Serial.print(value, 2);
        Serial.print(" -> ");
        Serial.print(clampedValue, 2);
        Serial.print(" (range: ");
        Serial.print(minVal, 2);
        Serial.print("-");
        Serial.print(maxVal, 2);
        Serial.println(")");
    }
    */

    _tracks[static_cast<size_t>(id)].setValue(stepIdx, clampedValue);
}

void ParameterManager::randomizeParameters()
{
    // Use a better random number generator
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

    for (size_t i = 0; i < static_cast<size_t>(ParamId::Count); ++i)
    {
        ParamId currentParamId = static_cast<ParamId>(i);

        // When randomizing, ensure the Slide parameter's length is set to max
        if (currentParamId == ParamId::Slide)
        {
            _tracks[i].stepCount = 16;
        }

        const auto &paramDef = CORE_PARAMETERS[i];
        float minVal = getFloatFromParameterValueType(paramDef.minValue);
        float maxVal = getFloatFromParameterValueType(paramDef.maxValue);
        std::uniform_real_distribution<float> distribution(minVal, maxVal);

        for (uint8_t step = 0; step < _tracks[i].stepCount; ++step)
        {
            switch (currentParamId)
            {
            case ParamId::Slide: {
                std::uniform_int_distribution<int> slide_dist(0, 12);
                _tracks[i].setValue(step, (slide_dist(generator) == 0) ? 1.0f : 0.0f);
                break;
            }
            case ParamId::Gate: {
                if ((step % 2) == 0) { // Even steps
                    // 75% chance of being 1
                    std::uniform_int_distribution<int> gate_dist(0, 3);
                    _tracks[i].setValue(step, (gate_dist(generator) == 0) ? 0.0f : 1.0f);
                } else { // Odd steps
                    // 33% chance of being 1
                    std::uniform_int_distribution<int> gate_dist(0, 2);
                    _tracks[i].setValue(step, (gate_dist(generator) == 0) ? 1.0f : 0.0f);
                }
                break;
            }
            case ParamId::GateLength: { // Corrected from GateSize
                std::uniform_real_distribution<float> dist(0.1f, 0.7f);
                _tracks[i].setValue(step, dist(generator));
                break;
            }
            case ParamId::Filter: {
                std::uniform_real_distribution<float> dist(0.2f, 0.7f);
                _tracks[i].setValue(step, dist(generator));
                break;
            }
            case ParamId::Attack: {
                std::uniform_real_distribution<float> dist(0.0f, 0.05f);
                _tracks[i].setValue(step, dist(generator));
                break;
            }
            case ParamId::Decay: {
                std::uniform_real_distribution<float> dist(0.08f, 0.5f);
                _tracks[i].setValue(step, dist(generator));
                break;
            }
            case ParamId::Note:
            case ParamId::Velocity:
            case ParamId::Octave:
            default: {
                _tracks[i].setValue(step, distribution(generator));
                break;
            }
            }
        }
    }
}