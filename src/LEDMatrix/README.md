# LED Matrix Module

This module controls the 8x8 WS2812B LED matrix for the Mudras Sequencer, providing real-time visual feedback for sequencer and UI activities, including playhead tracking, parameter editing, and visual themes. See the [main README](../../README.md) for overall project context.

---

## Overview

- Drives an 8x8 WS2812B (NeoPixel) LED matrix via FastLED.
- Visualizes sequencer state, playhead, parameter values, and user interactions.
- Integrates with parameter and theme logic for color-coded feedback.
- Modular design for easy adaptation to matrix size and layouts.

---

## Key Features

- Addressable per-LED color control with `setLED(x, y, color)`
- Full-matrix operations: clear, fill, direct buffer access
- Real-time feedback for sequence steps, selection, parameter editing, and idle states
- Theme support (distinct color mappings per parameter/interaction)
- Optimized for rapid updates and low latency

---

## Dependencies

- **[FastLED](https://github.com/FastLED/FastLED):** The FastLED library for colored LED animation on Arduino platforms.

---

## API Reference

| Method                             | Description                                          | Example                       |
|:------------------------------------|:-----------------------------------------------------|:------------------------------|
| `LEDMatrix()`                      | Construct new LEDMatrix object.                      | `LEDMatrix ledMatrix;`        |
| `void begin(uint8_t brightness)`    | Initialize FastLED and set brightness (default 64).  | `ledMatrix.begin(128);`       |
| `void setLED(int x, int y, color)` | Set color of single LED at (x, y).                   | `ledMatrix.setLED(3, 2, CRGB::Red);` |
| `void setAll(color)`                | Fill all pixels with one color.                      | `ledMatrix.setAll(CRGB::Blue);`      |
| `void show()`                       | Update matrix hardware with current buffer.          | `ledMatrix.show();`           |
| `void clear()`                      | Turn all LEDs off.                                   | `ledMatrix.clear();`          |
| `CRGB* getLeds()`                   | Access raw LED buffer.                               | `CRGB* leds = ledMatrix.getLeds();`  |

See [FastLED documentation](https://github.com/FastLED/FastLED) for details on `CRGB` types and matrix color handling.

---

## Usage Examples

```cpp
// Initialize matrix
LEDMatrix ledMatrix;
ledMatrix.begin(128); // Set brightness

// Set one LED red at (1,1)
ledMatrix.setLED(1, 1, CRGB::Red);
ledMatrix.show();

// Fill all LEDs with green
ledMatrix.setAll(CRGB::Green);
ledMatrix.show();

// Clear matrix
ledMatrix.clear();
ledMatrix.show();
```
For more examples, refer to the [FastLED examples](https://github.com/FastLED/FastLED/tree/master/examples).

---

## Feedback & Themes

The `LEDMatrixFeedback` class (see source) provides animated feedback for sequencer state, parameter edits, and theming. Each parameter is mapped to a distinct color based on the active visual theme for clear on-stage visibility.

Learn more in the [Sequencer Module README](../sequencer/README.md).

---

## Related Modules

- [Sequencer Module](../sequencer/README.md): Parameter/state logic driving LED feedback
- [Matrix Module](../matrix/README.md): User input to update sequencer state

---

## See Also

- [Main Project README](../../README.md)
- [Technical Manual](../../PROGRAMMERS_MANUAL.md)