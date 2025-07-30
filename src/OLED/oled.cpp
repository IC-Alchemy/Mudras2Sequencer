#include "oled.h"
#include "../../includes.h"
#include "../sequencer/SequencerDefs.h"
#include "../sequencer/ShuffleTemplates.h"
#include "../scales/scales.h"

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

    display.println("It smells like ");
    display.println("      Crazy  ");
    display.println("Can you hear it?");
    display.println("GET Crazy With it");
    display.display();
    delay(1000);
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

    const ParamButtonMapping* heldParam = getHeldParameterButton(uiState);

    if (heldParam != nullptr) {
        // Display parameter editing information
        uint8_t voice = uiState.isVoice2Mode ? 2 : 1;
        const Sequencer& currentSeq = uiState.isVoice2Mode ? seq2 : seq1;
        uint8_t currentStep = currentSeq.getCurrentStepForParameter(heldParam->paramId);
        float currentValue = currentSeq.getStepParameterValue(heldParam->paramId, currentStep);
        displayParameterInfo(heldParam->name, currentValue, voice, currentStep);
    } else if (uiState.selectedStepForEdit != -1) {
        // Show current step if step editing mode
        display.setCursor(15, 25);
        display.setTextSize(2);
        display.print("Edit Step: ");
        display.println(uiState.selectedStepForEdit + 1);
    } else {
        // Default screen: Show current scale and shuffle pattern
        display.setTextSize(1);

        // Display Scale Name
        extern uint8_t currentScale;
        display.setCursor(5, 15);
        display.print("Scale: ");
        display.println(scales[currentScale].name);

        // Display Shuffle Pattern
        extern const ShuffleTemplate shuffleTemplates[];
        display.setCursor(5, 35);
        display.print("Shuffle: ");
        display.println(shuffleTemplates[uiState.currentShufflePatternIndex].name);
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
            return String((int)calculateFilterFrequency(value)) + "Hz";
            
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