#include "ledMatrix.h"

// LEDMatrix implementation for an 8x4 WS2812B matrix

LEDMatrix::LEDMatrix() {
    clear();
}

void LEDMatrix::begin(uint8_t brightness) {
    // Initialize FastLED for the matrix
    Serial.print("LEDMatrix: Initializing with brightness: ");
    Serial.println(brightness);
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, WIDTH * HEIGHT);
    FastLED.setBrightness(brightness);
    clear();
    show();
}

void LEDMatrix::setLED(int x, int y, const CRGB& color) {
    // Set color of a single LED at (x, y)
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    leds[x + y * WIDTH] = color;
}

void LEDMatrix::setAll(const CRGB& color) {
    // Set all LEDs to the specified color
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        leds[i] = color;
    }
}

void LEDMatrix::show() {
    // Push LED data to the matrix
    FastLED.show();
}

void LEDMatrix::clear() {
    // Clear the matrix (set all LEDs to black)
    setAll(CRGB::Black);
}

CRGB* LEDMatrix::getLeds() {
    // Direct access to LED array (advanced use)
    return leds;
}
