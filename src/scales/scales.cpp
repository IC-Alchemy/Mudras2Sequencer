#include "scales.h"

const int major_intervals[] = {0, 2, 4, 5, 7, 9, 11};
const int minor_intervals[] = {0, 2, 3, 5, 7, 8, 10};

const Scale scales[] = {
    {"Major", major_intervals, 7},
    {"Minor", minor_intervals, 7}
};

const uint8_t num_scales = sizeof(scales) / sizeof(Scale);

uint8_t currentScale = 0; // Default to Major scale
