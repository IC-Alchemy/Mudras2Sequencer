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

void OLEDDisplay::update(const UIState& uiState, const Sequencer& seq1, const Sequencer& seq2) {
    if (!initialized) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Draw a border for a more professional look
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);

    // Handle settings mode display
    if (uiState.settingsMode) {
        displaySettingsMenu(uiState);
        display.display();
        return;
    }
    
    // Handle voice parameter editing mode display
    if (uiState.inVoiceParameterMode && (millis() - uiState.voiceParameterChangeTime < 3000)) {
        // Show voice parameter info for 3 seconds after change
        // Note: We need access to voiceManager, leadVoiceId, bassVoiceId from main file
        // For now, we'll show a placeholder - this will be updated when called from main
        display.setCursor(5, 5);
        display.setTextSize(1);
        display.print("VOICE PARAM MODE");
        display.setCursor(5, 20);
        display.print("Button: ");
        display.print(uiState.lastVoiceParameterButton);
        display.setCursor(5, 35);
        display.print("Voice: ");
        display.print(uiState.isVoice2Mode ? "2" : "1");
        display.display();
        return;
    } else if (uiState.inVoiceParameterMode) {
        // Clear voice parameter mode after timeout
        const_cast<UIState&>(uiState).inVoiceParameterMode = false;
    }

    const ParamButtonMapping* heldParam = getHeldParameterButton(uiState);

    if (heldParam != nullptr) {
        // Display parameter editing information
        uint8_t voice = uiState.isVoice2Mode ? 2 : 1;
        const Sequencer& currentSeq = uiState.isVoice2Mode ? seq2 : seq1;
        uint8_t currentStep = currentSeq.getCurrentStepForParameter(heldParam->paramId);
        float currentValue = currentSeq.getStepParameterValue(heldParam->paramId, currentStep);
        displayParameterInfo(heldParam->name, currentValue, voice, currentStep);
    } else if (uiState.selectedStepForEdit != -1) {
        // Step editing mode - show step parameter values
        if (uiState.currentEditParameter != ParamId::Count) {
            // Display the currently editing parameter for the selected step
            uint8_t voice = uiState.isVoice2Mode ? 2 : 1;
            const Sequencer& currentSeq = uiState.isVoice2Mode ? seq2 : seq1;
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
        display.print(uiState.isVoice2Mode ? "2" : "1");
        
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
        int currentPresetIndex = (uiState.settingsMenuIndex == 0) ? 
                               uiState.voice1PresetIndex : uiState.voice2PresetIndex;
        
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
        
        // Voice configurations with better visual hierarchy
        for (int i = 0; i < 2; i++) {
            int yPos = 20 + (i * 20);
            
            // Selection indicator
            if (uiState.settingsMenuIndex == i) {
                // Draw selection box
                display.drawRect(2, yPos - 2, SCREEN_WIDTH - 4, 18, SH110X_WHITE);
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
            display.setCursor(5, yPos + 8);
            const char* presetName = (i == 0) ? 
                VoicePresets::getPresetName(uiState.voice1PresetIndex) :
                VoicePresets::getPresetName(uiState.voice2PresetIndex);
            display.print("Preset: ");
            display.print(presetName);
        }
        
        // Instructions at bottom
        display.setCursor(5, 56);
        display.print("BTN1-2:SEL  BTN8:EDIT");
    }
}

void OLEDDisplay::displayVoiceParameterInfo(const UIState& uiState, VoiceManager* voiceManager, 
                                           uint8_t leadVoiceId, uint8_t bassVoiceId) {
    if (!initialized || !voiceManager) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    
    // Get current voice configuration
    uint8_t currentVoiceId = uiState.isVoice2Mode ? bassVoiceId : leadVoiceId;
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
    display.print(uiState.isVoice2Mode ? "2" : "1");
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