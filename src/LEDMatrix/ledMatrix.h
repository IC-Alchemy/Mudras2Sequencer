#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#include <Arduino.h>
#include <FastLED.h>

/**
 * @class LEDMatrix
 * @brief Controls an 8x8 WS2812B LED matrix.
 */
class LEDMatrix {
public:
    static constexpr uint8_t WIDTH = 8;
    static constexpr uint8_t HEIGHT = 8;
    static constexpr uint8_t DATA_PIN = 1;

    

    LEDMatrix();
    void begin(uint8_t brightness = 200);
    void setLED(int x, int y, const CRGB& color);
    void setAll(const CRGB& color);
    void show();
    void clear();

    // Optional: direct access for advanced use
    CRGB* getLeds();

private:
    CRGB leds[WIDTH * HEIGHT];
};

#endif // LEDMATRIX_H
