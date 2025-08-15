#ifndef SCALES_H
#define SCALES_H

#include <Arduino.h>

// Global scale data used across the application.
// NOTE: Synthesis components (e.g., Voice) should NOT access these globals directly.
//       Instead, inject references via setters (e.g., Voice::setScaleTable and
//       Voice::setCurrentScalePointer) to keep modules testable and decoupled.
//       These globals remain for UI/Sequencer modules that manage scale selection.
extern int scale[7][48];          // Scale tables: 7 scales, each with 48 step-to-semitone entries
extern const char* scaleNames[7]; // Human-readable scale names
extern uint8_t currentScale;      // Currently selected scale index (0..6)

#endif // SCALES_H
