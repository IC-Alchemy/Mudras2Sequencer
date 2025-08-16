#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <Arduino.h>

// UI Constants and mappings extracted from UIEventHandler
// Organized by purpose: button mappings, timing, layout.
// Musical/UI intent documented per group.

// =======================
// Button mappings
// =======================
// Primary control buttons for performance and navigation
constexpr uint8_t BUTTON_SLIDE_MODE = 22;            // Toggle slide edit mode
constexpr uint8_t BUTTON_TOGGLE_DELAY = 23;          // Toggle delay effect and set AS5600 to delay param
constexpr uint8_t BUTTON_VOICE_SWITCH = 24;          // Cycle selected voice 1..4
constexpr uint8_t BUTTON_AS5600_CONTROL = 25;        // Cycle AS5600 control target
constexpr uint8_t BUTTON_PLAY_STOP = 26;             // Start/stop transport; long-press opens settings when stopped
constexpr uint8_t BUTTON_CHANGE_SCALE = 27;          // Cycle musical scale
constexpr uint8_t BUTTON_CHANGE_THEME = 28;          // Cycle LED theme
constexpr uint8_t BUTTON_CHANGE_SWING_PATTERN = 29;  // Cycle shuffle/swing template

// Randomize buttons per voice (short: randomize, long: reset)
constexpr uint8_t BUTTON_RANDOMIZE_SEQ1 = 30;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ2 = 31;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ3 = 32;
constexpr uint8_t BUTTON_RANDOMIZE_SEQ4 = 33;

// Step buttons (pads) count
constexpr uint8_t NUMBER_OF_STEP_BUTTONS = 16;

// =======================
// Timing values
// =======================
// Long press threshold comes from ButtonManager to keep a single source
// Additional UI-related timing windows below
constexpr unsigned long AS5600_DOUBLE_PRESS_WINDOW = 300; // ms: window for double press on AS5600 control
constexpr unsigned long CONTROL_LED_FLASH_DURATION_MS = 250; // ms: brief confirmation flash for control actions

// =======================
// UI Layout / Behavior
// =======================
// Reserved slot for future layout constants if needed

#endif // UI_CONSTANTS_H