#ifndef UI_STATE_H
#define UI_STATE_H

#include <Arduino.h>
#include "../sequencer/SequencerDefs.h"
#include "../sensors/as5600.h" // For AS5600ParameterMode

// Forward declarations
class VoiceManager;
class OLEDDisplay;

/**
 * @brief Observer interface for voice parameter changes
 *
 * Components that need to be notified of voice parameter changes
 * should implement this interface and register with the notification system.
 */
class VoiceParameterObserver {
public:
    virtual ~VoiceParameterObserver() = default;

    /**
     * Called when a voice parameter is changed
     * @param voiceId ID of the voice that was modified
     * @param buttonIndex Button index that triggered the change (9-24)
     * @param parameterName Human-readable name of the parameter
     */
    virtual void onVoiceParameterChanged(uint8_t voiceId, uint8_t buttonIndex, const char* parameterName) = 0;
};

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
    bool delayOn = true;
    bool modGateParamSeqLengthsMode = false;
    bool slideMode = false;
    bool isVoice2Mode = false;
    int selectedStepForEdit = -1;
    ParamId currentEditParameter = ParamId::Count; // Parameter being edited in toggle mode (Count = none)
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
   uint8_t voice3PresetIndex = 1;      // Default to Lead Voice
   uint8_t voice4PresetIndex = 5;      //Default to Percussion Voice
   unsigned long playStopPressTime = 0;
   bool playStopWasPressed = false;
   
   // --- Voice Parameter Editing State ---
   bool inVoiceParameterMode = false;
   uint8_t lastVoiceParameterButton = 0;  // Track which voice parameter was last changed
   unsigned long voiceParameterChangeTime = 0;  // Timestamp of last voice parameter change

   // --- Voice Parameter Change Notification ---
   bool voiceParameterChanged = false;  // Flag to trigger immediate OLED update
   uint8_t changedVoiceId = 0;         // ID of voice that was changed
   const char* changedParameterName = nullptr;  // Name of parameter that was changed

   // --- Observer Registration ---
   VoiceParameterObserver* voiceParameterObserver = nullptr;  // Single observer for now

   /**
    * @brief Notify registered observer of voice parameter change
    * @param voiceId ID of the voice that was modified
    * @param buttonIndex Button index that triggered the change (9-24)
    * @param parameterName Human-readable name of the parameter
    */
   void notifyVoiceParameterChanged(uint8_t voiceId, uint8_t buttonIndex, const char* parameterName) {
       // Set flags for immediate OLED update
       voiceParameterChanged = true;
       changedVoiceId = voiceId;
       changedParameterName = parameterName;
       lastVoiceParameterButton = buttonIndex;
       voiceParameterChangeTime = millis();
       inVoiceParameterMode = true;

       // Notify observer if registered
       if (voiceParameterObserver) {
           voiceParameterObserver->onVoiceParameterChanged(voiceId, buttonIndex, parameterName);
       }
   }

   /**
    * @brief Clear voice parameter change flags after OLED update
    */
   void clearVoiceParameterChangeFlags() {
       voiceParameterChanged = false;
       changedParameterName = nullptr;
   }
};

#endif // UI_STATE_H