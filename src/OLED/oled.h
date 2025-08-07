#ifndef OLED_H
#define OLED_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "../ui/UIState.h"
#include "../ui/ButtonManager.h"
#include "../sequencer/Sequencer.h"
#include "../sequencer/SequencerDefs.h"

/**
 * @brief Observer interface for voice parameter changes
 * 
 * Provides immediate callback system for real-time OLED updates
 */
class VoiceParameterObserver {
public:
    virtual ~VoiceParameterObserver() = default;
    virtual void onVoiceParameterChanged(uint8_t voiceId, const VoiceState& state) = 0;
    virtual void onVoiceSwitched(uint8_t newVoiceId) = 0;
};

// OLED Configuration
#define OLED_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

/**
 * @brief OLED Display Manager for PicoMudrasSequencer
 * 
 * Provides real-time visual feedback for parameter editing,
 * showing which parameter button is held and its current value.
 * Implements VoiceParameterObserver for immediate updates.
 */
class OLEDDisplay : public VoiceParameterObserver {
public:
    /**
     * @brief Constructor
     */
    OLEDDisplay();
    
    /**
     * @brief Initialize the OLED display
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Update the display with current parameter information
     * @param uiState Current UI state containing button states
     * @param seq1 Sequencer 1 reference for parameter values
     * @param seq2 Sequencer 2 reference for parameter values
     */
    void update(const UIState& uiState, const Sequencer& seq1, const Sequencer& seq2);
    
    /**
     * @brief Update the display with voice manager access for settings mode
     * @param uiState Current UI state containing button states
     * @param seq1 Sequencer 1 reference for parameter values
     * @param seq2 Sequencer 2 reference for parameter values
     * @param voiceManager Pointer to voice manager for accessing voice configs
     */
    void update(const UIState& uiState, const Sequencer& seq1, const Sequencer& seq2, class VoiceManager* voiceManager);
    
    /**
     * @brief Clear the display
     */
    void clear();
    
    /**
     * @brief Check if display is initialized
     * @return true if display is ready
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Set the voice manager reference for parameter updates
     * @param voiceManager Pointer to the voice manager
     */
    void setVoiceManager(class VoiceManager* voiceManager);

    // VoiceParameterObserver interface implementation
    /**
     * @brief Callback for when a voice parameter changes
     * @param voiceId ID of the voice that changed
     * @param state Current voice state containing all parameter values
     */
    void onVoiceParameterChanged(uint8_t voiceId, const VoiceState& state) override;

    /**
     * @brief Callback for when voice is switched (interface requirement)
     * @param newVoiceId ID of the newly selected voice
     */
    void onVoiceSwitched(uint8_t newVoiceId) override;

    /**
     * @brief Handle voice switching with immediate OLED update (extended version)
     * @param uiState Current UI state
     * @param voiceManager Pointer to voice manager
     */
    void onVoiceSwitched(const UIState& uiState, class VoiceManager* voiceManager);

private:
    Adafruit_SH1106G display;
    bool initialized = false;
    
    /**
     * @brief Display parameter information
     * @param paramName Name of the parameter
     * @param currentValue Current parameter value
     * @param voice Voice number (1 or 2)
     * @param stepIndex Current step index for the parameter
     */
    void displayParameterInfo(const char* paramName, float currentValue,
                             uint8_t voice, uint8_t stepIndex);



    /**
     * @brief Format parameter value for display
     * @param paramId Parameter ID for formatting rules
     * @param value Raw parameter value
     * @return Formatted string for display
     */
    String formatParameterValue(ParamId paramId, float value);
    
    /**
     * @brief Format AS5600 parameter value for display
     * @param paramMode AS5600 parameter mode for formatting rules
     * @param value Raw parameter value
     * @return Formatted string for display
     */
    String formatAS5600ParameterValue(AS5600ParameterMode paramMode, float value);
    
    /**
     * @brief Get AS5600 parameter name for display
     * @param paramMode AS5600 parameter mode
     * @return Parameter name string
     */
    const char* getAS5600ParameterName(AS5600ParameterMode paramMode);
    
    /**
     * @brief Display settings menu
     * @param uiState Current UI state containing settings information
     */
    void displaySettingsMenu(const UIState& uiState);
    
    /**
     * @brief Display voice parameter information
     * @param uiState Current UI state containing voice parameter information
     * @param voiceManager Pointer to voice manager for accessing voice configs
     * @param leadVoiceId ID of the lead voice
     * @param bassVoiceId ID of the bass voice
     */
    void displayVoiceParameterInfo(const UIState& uiState, class VoiceManager* voiceManager, 
                                  uint8_t leadVoiceId, uint8_t bassVoiceId);
    
    /**
     * @brief Display voice parameter toggles in settings mode
     * @param uiState Current UI state
     * @param voiceManager Pointer to voice manager for accessing voice configs
     */
    void displayVoiceParameterToggles(const UIState& uiState, class VoiceManager* voiceManager);
    
    /**
     * @brief Force immediate update of the display for voice parameter changes
     * @param uiState Current UI state
     * @param voiceManager Pointer to voice manager for accessing voice configs
     */
    void forceUpdate(const UIState& uiState, class VoiceManager* voiceManager);
    
    /**
     * @brief Display AS5600 parameter selection and value information
     * @param uiState Current UI state containing AS5600 parameter information
     * @param parameterName Name of the currently selected AS5600 parameter
     * @param currentValue Current value of the AS5600 parameter (optional)
     * @param showValue Whether to display the current value or just the parameter name
     */
    void displayAS5600ParameterInfo(const UIState& uiState, const char* parameterName, 
                                   float currentValue = 0.0f, bool showValue = false);

private:
    class VoiceManager* voiceManagerRef = nullptr; // Reference to voice manager for immediate updates

};

// External declaration for the global OLED display instance
// (defined in PicoMudrasSequencer.ino)
extern OLEDDisplay oledDisplay;

#endif // OLED_H