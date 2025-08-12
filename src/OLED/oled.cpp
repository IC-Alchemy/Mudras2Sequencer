#include "oled.h"
#include "../voice/Voice.h"
#include "../../includes.h"
#include "../sequencer/SequencerDefs.h"
#include "../sequencer/ShuffleTemplates.h"
#include "../scales/scales.h"
#include "../ui/ButtonManager.h"

// Constructor implementation
OLEDDisplay::OLEDDisplay() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), initialized(false) {
}

bool OLEDDisplay::begin() {
    // Initialize the display with the I2C address
    if (!display.begin(OLED_I2C_ADDRESS, true)) {
        Serial.println("[ERROR] OLED display initialization failed!");
        return false;
    }

    initialized = true;

    // Clear the display and show startup message
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("You did it, Here we go");    delay(500);

    Serial.println("OLED display initialized successfully");
    return true;
}

void OLEDDisplay::clear() {
    if (!initialized) return;

    display.clearDisplay();
    display.display();
}

void OLEDDisplay::setVoiceManager(VoiceManager* voiceManager) {
    voiceManagerRef = voiceManager;
    Serial.println("OLED: Voice manager reference set");
}

void OLEDDisplay::displayVoiceParameterToggles(const UIState& uiState, VoiceManager* voiceManager) {
    if (!initialized || !voiceManager) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Draw border for professional look
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);

    // Compact header with current voice indicator
    display.setCursor(2, 2);
    display.print("VOICE PARAMS - EDITING V");
    display.print(uiState.selectedVoiceIndex + 1);

    // Draw separator line
    display.drawFastHLine(2, 10, SCREEN_WIDTH - 4, SH110X_WHITE);

    // Get external voice IDs
    extern uint8_t leadVoiceId;
    extern uint8_t bassVoiceId;

    extern uint8_t voice3Id;
    extern uint8_t voice4Id;

    // Compact parameter display for 2 voices
    const char* paramNames[] = {"E", "O", "W", "F", "R", "D"}; // Single letters
    const int paramButtons[] = {9, 10, 11, 12, 13, 14};
    const int numParams = 6;

    // Voice IDs to display
    uint8_t voiceIds[] = {leadVoiceId, bassVoiceId, voice3Id, voice4Id};
    const char* voiceLabels[] = {"V1", "V2", "V3", "V4"};

    // Show parameters for all voices in compact grid (2 rows x 2 columns)
    for (int voiceIndex = 0; voiceIndex < 4; voiceIndex++) {
        uint8_t voiceId = voiceIds[voiceIndex];
        VoiceConfig* config = voiceManager->getVoiceConfig(voiceId);
        if (!config) continue;

        // Layout: two rows (y=13, y=31) and two columns (x offsets)
        int row = voiceIndex / 2;  // 0 or 1
        int col = voiceIndex % 2;  // 0 or 1
        int yStart = 13 + (row * 18);
        int xStart = (col == 0) ? 2 : 66; // split screen

        // Voice header with selection indicator
        display.setCursor(xStart, yStart);
        bool isCurrentVoice = (voiceIndex == uiState.selectedVoiceIndex);

        if (isCurrentVoice) {
            display.print("*");  // Indicate currently selected voice being edited
        } else {
            display.print(" ");
        }
        display.print(voiceLabels[voiceIndex]);

        // Parameter states in compact format
        for (int i = 0; i < numParams; i++) {
            int xPos = xStart + 10 + (i * 10); // denser per-voice grid
            display.setCursor(xPos, yStart);

            // Get parameter state
            bool paramState = false;
            switch (paramButtons[i]) {
                case 9: paramState = config->hasEnvelope; break;
                case 10: paramState = config->hasOverdrive; break;
                case 11: paramState = config->hasWavefolder; break;
                case 12:
                    // Show filter mode as number (0-4)
                    display.print(static_cast<int>(config->filterMode));
                    continue;
                case 13:
                    // Show resonance as 2-digit percentage
                    display.print((int)(config->filterRes * 100));
                    continue;
                case 14: paramState = config->hasDalek; break;
            }

            // Display compact ON/OFF state
            if (paramButtons[i] != 12 && paramButtons[i] != 13) {
                display.print(paramState ? "1" : "0");
            }
        }

        // Parameter labels on second line (single letters)
        for (int i = 0; i < numParams; i++) {
            int xPos = xStart + 10 + (i * 10);
            display.setCursor(xPos, yStart + 8);
            display.print(paramNames[i]);
        }
    }

    // Compact instructions at bottom
    display.setCursor(2, 56);
    display.print("9-14:TOG *=EDITING 8:BACK");

    display.display();
}

void OLEDDisplay::update(const UIState& uiState, const Sequencer& seq1, const Sequencer& seq2, const Sequencer& seq3, const Sequencer& seq4) {
    // Call the overloaded version with nullptr for voiceManager
    update(uiState, seq1, seq2, seq3, seq4, nullptr);
}

void OLEDDisplay::update(const UIState& uiState, const Sequencer& seq1, const Sequencer& seq2, const Sequencer& seq3, const Sequencer& seq4, VoiceManager* voiceManager) {
    if (!initialized) return;

    // Store voice manager reference for immediate updates
    voiceManagerRef = voiceManager;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Draw a border for a more professional look
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);

    // Priority-based display logic to prevent menu conflicts

    // 1. HIGHEST PRIORITY: Voice parameter editing mode (when in settings and recently changed parameters)
    if (uiState.settingsMode && voiceManager && uiState.inVoiceParameterMode &&
        (millis() - uiState.voiceParameterChangeTime < 5000)) {
        displayVoiceParameterToggles(uiState, voiceManager);
        display.display();
        return;
    }

    // 2. MEDIUM PRIORITY: Settings mode (main settings menu or preset selection)
    if (uiState.settingsMode) {
        displaySettingsMenu(uiState);
        display.display();
        return;
    }

    // 3. LOW PRIORITY: Voice parameter info display (outside of settings mode)
    if (uiState.inVoiceParameterMode && (millis() - uiState.voiceParameterChangeTime < 3000)) {
        // Show voice parameter info for 3 seconds after change
        display.setCursor(5, 5);
        display.setTextSize(1);
        display.print("VOICE PARAM MODE");
        display.setCursor(5, 20);
        display.print("Button: ");
        display.print(uiState.lastVoiceParameterButton);
        display.setCursor(5, 35);
        display.print("Voice: ");
        display.print(uiState.selectedVoiceIndex + 1);
        display.display();
        return;
    } else if (uiState.inVoiceParameterMode) {
        // Clear voice parameter mode after timeout
        const_cast<UIState&>(uiState).inVoiceParameterMode = false;
    }

    const ParamButtonMapping* heldParam = getHeldParameterButton(uiState);

    if (heldParam != nullptr) {
        // Display parameter editing information
        uint8_t voice = uiState.selectedVoiceIndex + 1;
        const Sequencer& currentSeq = (uiState.selectedVoiceIndex == 0) ? seq1 :
                                      (uiState.selectedVoiceIndex == 1) ? seq2 :
                                      (uiState.selectedVoiceIndex == 2) ? seq3 : seq4;
        uint8_t currentStep = currentSeq.getCurrentStepForParameter(heldParam->paramId);
        float currentValue = currentSeq.getStepParameterValue(heldParam->paramId, currentStep);
        displayParameterInfo(heldParam->name, currentValue, voice, currentStep);
    } else if (uiState.selectedStepForEdit != -1) {
        // Step editing mode - show step parameter values
        if (uiState.currentEditParameter != ParamId::Count) {
            // Display the currently editing parameter for the selected step
            uint8_t voice = uiState.selectedVoiceIndex + 1;
            const Sequencer& currentSeq = (uiState.selectedVoiceIndex == 0) ? seq1 :
                                          (uiState.selectedVoiceIndex == 1) ? seq2 :
                                          (uiState.selectedVoiceIndex == 2) ? seq3 : seq4;
            float currentValue = currentSeq.getStepParameterValue(uiState.currentEditParameter, uiState.selectedStepForEdit);

            // Find parameter name
            const char* paramName = "Unknown";
            for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
                if (PARAM_BUTTON_MAPPINGS[i].paramId == uiState.currentEditParameter) {
                    paramName = PARAM_BUTTON_MAPPINGS[i].name;
                    break;
                }
            }

            displayParameterInfo(paramName, currentValue, voice, uiState.selectedStepForEdit);
        } else {
            // No parameter selected - show step selection prompt
            display.setCursor(5, 20);
            display.setTextSize(2);
            display.print("Step ");
            display.print(uiState.selectedStepForEdit + 1);

            display.setCursor(5, 40);
            display.setTextSize(1);
            display.print("Press param button");
            display.setCursor(5, 50);
            display.print("to edit");
        }
    } else {
        // Default screen: Show current scale and shuffle pattern with enhanced formatting
        display.setTextSize(1);

        // Header with title
        display.setCursor(25, 5);
        display.setTextSize(1);
        display.print("Mudras Sequencer");

        // Horizontal line under header
        display.drawFastHLine(5, 14, SCREEN_WIDTH - 10, SH110X_WHITE);

        // Scale section

        display.setCursor(5, 20);
        display.print("Scale:");

        display.setCursor(55, 20);
        display.setTextSize(1);
        display.print(scaleNames[currentScale]);

        // Separator line
       // display.drawFastHLine(10, 30, SCREEN_WIDTH - 20, SH110X_WHITE);

        // Shuffle section

        display.setCursor(5, 36);
        display.print("Shuffle:");

        display.setCursor(65, 36);
        display.print(getShuffleTemplateName(uiState.currentShufflePatternIndex));

        // Bottom status line
     //   display.drawFastHLine(5, 48, SCREEN_WIDTH - 10, SH110X_WHITE);
        display.setCursor(5, 52);
        display.setTextSize(1);
        display.print("Voice: ");
        display.print(uiState.selectedVoiceIndex + 1);

        // Current step indicator
        display.setCursor(70, 52);
    }

    display.display();
}

void OLEDDisplay::displayParameterInfo(const char* paramName, float currentValue,
                                      uint8_t voice, uint8_t stepIndex) {
    // Parameter name
    display.setCursor(5, 5);
    display.setTextSize(2);
    display.print(paramName);

    // Voice indicator
    display.setTextSize(1);
    display.setCursor(100, 5);
    display.print("V");
    display.print(voice);

    // Current step
    display.setCursor(100, 15);
    display.print("S");
    display.print(stepIndex + 1);

    // Separator line
    display.drawFastHLine(2, 24, SCREEN_WIDTH - 4, SH110X_WHITE);

    // Parameter value
    display.setTextSize(2);
    display.setCursor(5, 32);

    // Format the parameter value based on its type
    ParamId paramId = ParamId::Note; // Default

    // Find the ParamId based on the parameter name
    for (size_t i = 0; i < PARAM_BUTTON_MAPPINGS_SIZE; ++i) {
        if (strcmp(PARAM_BUTTON_MAPPINGS[i].name, paramName) == 0) {
            paramId = PARAM_BUTTON_MAPPINGS[i].paramId;
            break;
        }
    }

    String formattedValue = formatParameterValue(paramId, currentValue);
    display.print(formattedValue);

    // Progress bar for normalized parameters
    if (paramId != ParamId::Note && paramId != ParamId::Octave &&
        paramId != ParamId::Gate && paramId != ParamId::Slide) {

        // Draw progress bar
        int barWidth = SCREEN_WIDTH - 10;
        int barHeight = 10;
        int barX = 5;
        int barY = 52;

        // Background
        display.drawRect(barX, barY, barWidth, barHeight, SH110X_WHITE);

        // Fill based on parameter value (0.0 to 1.0)
        int fillWidth = (int)(currentValue * (barWidth - 4));
        if (fillWidth > 0) {
            display.fillRect(barX + 2, barY + 2, fillWidth, barHeight - 4, SH110X_WHITE);
        }
    }
}

String OLEDDisplay::formatParameterValue(ParamId paramId, float value) {
    switch (paramId) {
        case ParamId::Note:
            return String((int)value);

        case ParamId::Velocity:
            return String((int)(value * 100)) + "%";

        case ParamId::Filter:
        {
            int filterFreq =  daisysp::fmap(value, 100.0f, 6710.0f, daisysp::Mapping::EXP);
            return String((int)(filterFreq)) + "Hz";
        }

        case ParamId::Attack:
            return String(value, 3) + "s";

        case ParamId::Decay:
            return String(value, 3) + "s";

        case ParamId::Octave:
            if (value < 0.15f) return "-1";
            else if (value > 0.4f) return "+1";
            else return "0";

        case ParamId::GateLength:
            return String((int)(value * 100)) + "%";

        case ParamId::Gate:
            return value > 0.5f ? "ON" : "OFF";

        case ParamId::Slide:
            return value > 0.5f ? "ON" : "OFF";

        default:
            return String(value, 2);
    }
}

void OLEDDisplay::displaySettingsMenu(const UIState& uiState) {
    display.setTextSize(1);

    if (uiState.inPresetSelection) {
        // Enhanced preset selection with cycling interface
        int currentPresetIndex = (uiState.settingsMenuIndex == 0) ? uiState.voice1PresetIndex :
                                (uiState.settingsMenuIndex == 1) ? uiState.voice2PresetIndex :
                                (uiState.settingsMenuIndex == 2) ? uiState.voice3PresetIndex : uiState.voice4PresetIndex;

        // Header with voice info
        display.setCursor(5, 5);
        display.print("VOICE ");
        display.print(uiState.settingsMenuIndex + 1);
        display.print(" PRESET");

        // Draw separator line
        display.drawFastHLine(5, 14, SCREEN_WIDTH - 10, SH110X_WHITE);

        // Current preset - large and centered
        display.setTextSize(2);
        const char* currentPresetName = VoicePresets::getPresetName(currentPresetIndex);
        int textWidth = strlen(currentPresetName) * 12; // Approximate width for size 2
        int centerX = (SCREEN_WIDTH - textWidth) / 2;
        display.setCursor(centerX, 20);
        display.print(currentPresetName);

        // Navigation indicators
        display.setTextSize(1);

        // Previous preset (if available)
        if (currentPresetIndex > 0) {
            display.setCursor(5, 45);
            display.print("< ");
            display.print(VoicePresets::getPresetName(currentPresetIndex - 1));
        }

        // Next preset (if available)
        if (currentPresetIndex < VoicePresets::getPresetCount() - 1) {
            const char* nextPresetName = VoicePresets::getPresetName(currentPresetIndex + 1);
            int nextTextWidth = strlen(nextPresetName) * 6 + 12; // 6 pixels per char + "> " width
            display.setCursor(SCREEN_WIDTH - nextTextWidth, 45);
            display.print(nextPresetName);
            display.print(" >");
        }

        // Preset counter at bottom
        display.setCursor(5, 56);
        display.print(currentPresetIndex + 1);
        display.print("/");
        display.print(VoicePresets::getPresetCount());

        // Instructions
        display.setCursor(SCREEN_WIDTH - 84, 56); // Right-aligned
        display.print("BTN1-6:SEL BTN8:OK");

    } else {
        // Enhanced main settings menu
        display.setCursor(5, 5);
        display.setTextSize(1);
        display.print("SETTINGS MENU");

        // Draw separator line
        display.drawFastHLine(5, 14, SCREEN_WIDTH - 10, SH110X_WHITE);

        // Voice configurations with better visual hierarchy (4 voices)
        for (int i = 0; i < 4; i++) {
            int yPos = 20 + (i * 10);

            // Selection indicator
            if (uiState.settingsMenuIndex == i) {
                // Draw selection box
                display.drawRect(2, yPos - 2, SCREEN_WIDTH - 4, 9, SH110X_WHITE);
                display.setCursor(5, yPos);
                display.print("> ");
            } else {
                display.setCursor(5, yPos);
                display.print("  ");
            }

            // Voice label
            display.print("VOICE ");
            display.print(i + 1);

            // Current preset name
            display.setCursor(58, yPos);
            const char* presetName = (i == 0) ? VoicePresets::getPresetName(uiState.voice1PresetIndex) :
                                     (i == 1) ? VoicePresets::getPresetName(uiState.voice2PresetIndex) :
                                     (i == 2) ? VoicePresets::getPresetName(uiState.voice3PresetIndex) :
                                                VoicePresets::getPresetName(uiState.voice4PresetIndex);
            display.print("Preset: ");
            display.print(presetName);
        }

        // Instructions at bottom
        display.setCursor(5, 56);
        display.print("B 1-2:SEL  B 8:EDIT  B 9-24:PARAMS");
    }
}

void OLEDDisplay::displayVoiceParameterInfo(const UIState& uiState, VoiceManager* voiceManager,
                                           uint8_t leadVoiceId, uint8_t bassVoiceId) {
    if (!initialized || !voiceManager) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Get current voice configuration
    extern uint8_t voice3Id;
    extern uint8_t voice4Id;
    uint8_t selected = uiState.selectedVoiceIndex;
    uint8_t currentVoiceId = (selected == 0) ? leadVoiceId : (selected == 1) ? bassVoiceId : (selected == 2) ? voice3Id : voice4Id;
    VoiceConfig* config = voiceManager->getVoiceConfig(currentVoiceId);

    if (!config) {
        display.setCursor(5, 20);
        display.print("Voice config error");
        display.display();
        return;
    }

    // Header
    display.setCursor(5, 5);
    display.setTextSize(1);
    display.print("VOICE ");
    display.print(selected + 1);
    display.print(" PARAMETERS");

    // Draw separator line
    display.drawFastHLine(5, 14, SCREEN_WIDTH - 10, SH110X_WHITE);

    // Parameter information based on button pressed
    const char* paramName = "";
    String paramValue = "";

    switch (uiState.lastVoiceParameterButton) {
        case 9:
            paramName = "Envelope";
            paramValue = config->hasEnvelope ? "ON" : "OFF";
            break;
        case 10:
            paramName = "Overdrive";
            paramValue = config->hasOverdrive ? "ON" : "OFF";
            break;
        case 11:
            paramName = "Wavefolder";
            paramValue = config->hasWavefolder ? "ON" : "OFF";
            break;
        case 12:
            {
                paramName = "Filter Mode";
                const char* filterNames[] = {"LP12", "LP24", "LP36", "BP12", "BP24"};
                int mode = static_cast<int>(config->filterMode);
                if (mode >= 0 && mode < 5) {
                    paramValue = filterNames[mode];
                } else {
                    paramValue = "Unknown";
                }
            }
            break;
        case 13:
            paramName = "Filter Res";
            paramValue = String(config->filterRes, 2);
            break;
        default:
            paramName = "Parameter";
            paramValue = String(uiState.lastVoiceParameterButton);
            break;
    }

    // Display parameter name
    display.setCursor(5, 20);
    display.setTextSize(1);
    display.print(paramName);
    display.print(":");

    // Display parameter value
    display.setCursor(5, 35);
    display.setTextSize(2);
    display.print(paramValue);

    // Show button number
    display.setTextSize(1);
    display.setCursor(5, 55);
    display.print("Button ");
    display.print(uiState.lastVoiceParameterButton);

    display.display();
}

void OLEDDisplay::forceUpdate(const UIState& uiState, VoiceManager* voiceManager) {
    if (!initialized) {
        Serial.println("OLED: Force update failed - display not initialized");
        return;
    }

    if (!voiceManager) {
        Serial.println("OLED: Force update failed - voiceManager is null");
        return;
    }

    // Store voice manager reference
    voiceManagerRef = voiceManager;


    // Force immediate display update for voice parameter changes
    if (uiState.settingsMode && (uiState.inVoiceParameterMode ||
        (uiState.voiceParameterChangeTime > 0 && (millis() - uiState.voiceParameterChangeTime < 5000)))) {
        Serial.println("OLED: Conditions met - updating display");

        // Display voice parameter toggles immediately (this handles its own display setup)
        displayVoiceParameterToggles(uiState, voiceManager);
        display.display();

        Serial.println("OLED: Force update completed - displaying voice parameter toggles");
    } else {
        Serial.println("OLED: Conditions not met for force update");
    }
}

void OLEDDisplay::onVoiceParameterChanged(uint8_t voiceId, const VoiceState& state) {
    // This method is called immediately when a voice parameter changes
    // Provides immediate visual feedback with proper voice ID mapping

    if (!initialized) {
        Serial.println("OLED: Parameter change ignored - display not initialized");
        return;
    }

    if (!voiceManagerRef) {
        Serial.println("OLED: Parameter change ignored - no voice manager reference");
        return;
    }

    // Get external voice IDs for proper mapping
    extern uint8_t leadVoiceId;
    extern uint8_t bassVoiceId;

    // Determine which voice this corresponds to (Voice 1 or Voice 2)
    uint8_t displayVoiceNumber = 0;
    if (voiceId == leadVoiceId) {
        displayVoiceNumber = 1;
    } else if (voiceId == bassVoiceId) {
        displayVoiceNumber = 2;
    } else {
        Serial.print("OLED: Warning - Unknown voice ID: ");
        Serial.print(voiceId);
        Serial.print(" (leadVoiceId: ");
        Serial.print(leadVoiceId);
        Serial.print(", bassVoiceId: ");
        Serial.print(bassVoiceId);
        Serial.println(")");
        return;
    }

    // Comprehensive debug output for troubleshooting
    Serial.println("=== OLED Voice Parameter Change ===");
    Serial.print("Voice ID: ");
    Serial.print(voiceId);
    Serial.print(" -> Display Voice: ");
    Serial.println(displayVoiceNumber);
    Serial.print("Note: ");
    Serial.print(state.note);
    Serial.print(" Velocity: ");
    Serial.print(state.velocity);
    Serial.print(" Filter: ");
    Serial.print(state.filter);
    Serial.print(" Attack: ");
    Serial.print(state.attack);
    Serial.print(" Decay: ");
    Serial.println(state.decay);
    Serial.print("Lead Voice ID: ");
    Serial.println(leadVoiceId);
    Serial.print("Bass Voice ID: ");
    Serial.println(bassVoiceId);
    Serial.println("=================================");

    // Force immediate display update if we have the voice manager
     // This will trigger the display to show the current parameter states
     if (voiceManagerRef) {
         Serial.println("OLED: Triggering immediate display refresh");
         // Note: The actual display update will be handled by the main loop
         // calling forceUpdate() with the current UIState
     }
}

void OLEDDisplay::onVoiceSwitched(const UIState& uiState, VoiceManager* voiceManager) {
    if (!initialized) {
        Serial.println("OLED: Voice switch ignored - display not initialized");
        return;
    }

    if (!voiceManager) {
        Serial.println("OLED: Voice switch ignored - no voice manager");
        return;
    }

    // Store voice manager reference
    voiceManagerRef = voiceManager;

    // Get external voice IDs for proper mapping
    extern uint8_t leadVoiceId;
    extern uint8_t bassVoiceId;
    extern uint8_t voice3Id;
    extern uint8_t voice4Id;

    uint8_t selected = uiState.selectedVoiceIndex;
    uint8_t currentVoiceId = (selected == 0) ? leadVoiceId : (selected == 1) ? bassVoiceId : (selected == 2) ? voice3Id : voice4Id;
    uint8_t displayVoiceNumber = selected + 1;

    // Comprehensive debug output for voice switching
    Serial.println("=== OLED Voice Switch (Button 24) ===");
    Serial.print("Switched to Voice: ");
    Serial.println(displayVoiceNumber);
    Serial.print("Voice ID: ");
    Serial.println(currentVoiceId);
    Serial.print("Lead Voice ID: ");
    Serial.println(leadVoiceId);
    Serial.print("Bass Voice ID: ");
    Serial.println(bassVoiceId);
    Serial.print("Settings Mode: ");
    Serial.println(uiState.settingsMode);
    Serial.println("====================================");

    // Force immediate update if in settings mode to show voice parameter toggles
    if (uiState.settingsMode) {
        Serial.println("OLED: Forcing immediate update for voice switch in settings mode");
        displayVoiceParameterToggles(uiState, voiceManager);
        display.display();
        Serial.println("OLED: Voice switch display update completed");
    } else {
        Serial.println("OLED: Voice switch noted - will update on next regular refresh");
    }
}

void OLEDDisplay::onVoiceSwitched(uint8_t newVoiceId) {
    // Interface-compliant method for VoiceParameterObserver
    // This is called when the voice system switches to a new voice

    if (!initialized) {
        Serial.println("OLED: Voice switch ignored - display not initialized");
        return;
    }

    Serial.print("OLED: Voice switched to ID ");
    Serial.println(newVoiceId);

    // This method provides the minimal interface compliance
    // The extended version with UIState and VoiceManager parameters
    // should be used for full functionality
}