#include "LEDController.h"
#include "../sensors/as5600.h"

// External sensor instance
extern AS5600Sensor as5600Sensor;

// =======================
//   INITIALIZATION
// =======================

void initLEDController()
{
    // Currently no initialization required
}

// =======================
//   MAIN LED UPDATE FUNCTION
// =======================
void updateControlLEDs(LEDMatrix &ledMatrix, const UIState& uiState)
{
    const LEDThemeColors *activeThemeColors = getActiveThemeColors();
    const unsigned long currentTime = millis();
    const uint8_t pulseValue = ControlLEDs::PULSE_BASE_BRIGHTNESS + (sinf(currentTime * ControlLEDs::PULSE_FREQUENCY) * ControlLEDs::PULSE_AMPLITUDE);

    const CRGB delayIndicatorColor = CRGB(0, 166, 55);


    bool isAS5600Selected = (uiState.currentAS5600Parameter != AS5600ParameterMode::COUNT);

    auto isCurrentAS5600Button = [&](int buttonIndex) -> bool
    {
        switch (uiState.currentAS5600Parameter)
        {
            case AS5600ParameterMode::Velocity: return buttonIndex == ControlLEDs::VELOCITY_LED;
            case AS5600ParameterMode::Filter:   return buttonIndex == ControlLEDs::FILTER_LED;
            case AS5600ParameterMode::Attack:   return buttonIndex == ControlLEDs::ATTACK_LED;
            case AS5600ParameterMode::Decay:    return buttonIndex == ControlLEDs::DECAY_LED;
            default: return false;
        }
    };

    const struct ParamLEDConfig {
        int linearLedIdx;
        ParamId paramId;
        CRGB LEDThemeColors::*colorHeld;
        CRGB LEDThemeColors::*colorIdle;
    } paramLEDs[] = {
        {ControlLEDs::NOTE_LED,     ParamId::Note,     &LEDThemeColors::modNoteActive,     &LEDThemeColors::modNoteInactive},
        {ControlLEDs::VELOCITY_LED, ParamId::Velocity, &LEDThemeColors::modVelocityActive, &LEDThemeColors::modVelocityInactive},
        {ControlLEDs::FILTER_LED,   ParamId::Filter,   &LEDThemeColors::modFilterActive,   &LEDThemeColors::modFilterInactive},
        {ControlLEDs::ATTACK_LED,   ParamId::Attack,   &LEDThemeColors::modAttackActive,   &LEDThemeColors::modAttackInactive},
        {ControlLEDs::DECAY_LED,    ParamId::Decay,    &LEDThemeColors::modDecayActive,    &LEDThemeColors::modDecayInactive},
        {ControlLEDs::OCTAVE_LED,   ParamId::Octave,   &LEDThemeColors::modOctaveActive,   &LEDThemeColors::modOctaveInactive},
        {ControlLEDs::SLIDE_LED,    ParamId::Slide,    &LEDThemeColors::modSlideActive,   &LEDThemeColors::modSlideInactive},
    };

    auto setLEDByIndex = [&ledMatrix](int linearIdx, const CRGB &color) {
        ledMatrix.setLED(linearIdx % LEDMatrix::WIDTH, linearIdx / LEDMatrix::WIDTH, color);
    };

    auto createPulsedColor = [pulseValue](const CRGB &baseColor) {
        CRGB result = baseColor;
        return result.nscale8(pulseValue);
    };

    auto createFadedColor = [](const CRGB &baseColor, float paramValue) {
        CRGB result = baseColor;
        return result.nscale8(static_cast<uint8_t>(paramValue * 255.0f));
    };

 

    for (const auto &btn : paramLEDs) {
        CRGB color;
        bool isHeld = (btn.paramId == ParamId::Slide) ? uiState.slideMode : uiState.parameterButtonHeld[static_cast<int>(btn.paramId)];

        if (isHeld) {
            color = createPulsedColor(activeThemeColors->*(btn.colorHeld));
        } else if (isAS5600Selected && as5600Sensor.isConnected() && isCurrentAS5600Button(btn.linearLedIdx)) {
            float paramValue = getAS5600ParameterValue();
            color = createFadedColor(activeThemeColors->*(btn.colorHeld), paramValue);
        } else {
            color = activeThemeColors->*(btn.colorIdle);
        }
        setLEDByIndex(btn.linearLedIdx, color);
    }

    CRGB delayTimeColor = CRGB(0, 44, 33);
    CRGB delayFeedbackColor = CRGB(0, 55, 22);

    if (as5600Sensor.isConnected()) {
        float paramValue = getAS5600ParameterValue();
        switch (uiState.currentAS5600Parameter) {
            case AS5600ParameterMode::DelayTime:    delayTimeColor = createFadedColor(delayIndicatorColor, paramValue); break;
            case AS5600ParameterMode::DelayFeedback:delayFeedbackColor = createFadedColor(delayIndicatorColor, paramValue); break;
          
            default: break;
        }
    }

    setLEDByIndex(ControlLEDs::DELAY_TIME_LED, delayTimeColor);
    setLEDByIndex(ControlLEDs::DELAY_FEEDBACK_LED, delayFeedbackColor);


    setLEDByIndex(ControlLEDs::VOICE1_LED, uiState.isVoice2Mode ? CRGB::Black : activeThemeColors->defaultActive);
    setLEDByIndex(ControlLEDs::VOICE2_LED, uiState.isVoice2Mode ? activeThemeColors->defaultInactive : CRGB::Black);

    if (uiState.flash23Until && currentTime < uiState.flash23Until) {
        setLEDByIndex(ControlLEDs::DELAY_TOGGLE_LED, activeThemeColors->randomizeFlash);
    } else {
        setLEDByIndex(ControlLEDs::DELAY_TOGGLE_LED, uiState.delayOn ? activeThemeColors->gateOnV1 : activeThemeColors->gateOffV1);
    }

    if (uiState.flash31Until && currentTime < uiState.flash31Until) {
        setLEDByIndex(ControlLEDs::RANDOMIZE_LED, activeThemeColors->randomizeFlash);
    } else {
        setLEDByIndex(ControlLEDs::RANDOMIZE_LED, activeThemeColors->randomizeIdle);
    }
}
