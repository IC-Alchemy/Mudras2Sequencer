#ifndef OLED_H
#define OLED_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "../ui/UIState.h"
#include "../ui/ButtonManager.h"
#include "../sequencer/Sequencer.h"

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
 */
class OLEDDisplay {
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

};

// External declaration for the global OLED display instance
// (defined in PicoMudrasSequencer.ino)
extern OLEDDisplay oledDisplay;

#endif // OLED_H