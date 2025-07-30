#ifndef SCALES_H
#define SCALES_H

#include <Arduino.h>

struct Scale {
    const char* name;
    const int* intervals;
    uint8_t count;
};

extern const Scale scales[];
extern const uint8_t num_scales;

#endif // SCALES_H
