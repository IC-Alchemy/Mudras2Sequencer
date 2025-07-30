#ifndef SHUFFLE_TEMPLATES_H
#define SHUFFLE_TEMPLATES_H

#include <Arduino.h>

const int NUM_SHUFFLE_TEMPLATES = 16;
const int SHUFFLE_TEMPLATE_SIZE = 16; // Explicitly define template size

struct ShuffleTemplate
{
    const char *name;
    int8_t ticks[SHUFFLE_TEMPLATE_SIZE];
};

// 16-step groove templates for 480 PPQN.
// Each 16th note step = 120 ticks.
// A positive value delays the step, a negative value pushes it earlier.
const ShuffleTemplate shuffleTemplates[NUM_SHUFFLE_TEMPLATES] = {

{"No Shuffle", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
{"Light Swing (54%)", {0, 21, 0, 23, 0, 21, 0, 23, 0, 21, 0, 23, 0, 20, 0, 20}},
{"Medium Swing (58%)", {0, 30, 0, 33, 0, 30, 0, 30, 0, 33, 0, 30, 0, 33, 0, 30}},
{"Heavy Swing (62%)", {0, 35, 0, 40, 0, 35, 0, 40, 0, 35, 0, 40, 0, 35, 0, 40}},
{"Push/Pull", {0, 25, 0, -25, 0, 25, 0, -25, 0, 25, 0, -25, 0, 25, 0, -25}},
{"Humanize 1", {0, -5, 0, -10, 0, 12, 0, 10, 0, -9, 0, -10, 0, 15, 0, 10}},
{"Humanize 2", {0, -4, 8, 5, 0, 0, 0, -5, 0, -3, 3, 5, 0, 0, 9, -5}},
{"Hip-Hop", {0, 30, 0, 10, 0, 30, 0, 10, 0, 30, 0, 10, 0, 30, 0, 10}},
{"Funk Groove", {0, 40, 0, -10, 0, 30, 0, 20, 0, 40, 0, -10, 0, 30, 0, 20}},
{"Latin Tumbao", {0, 0, 25, 0, 0, 0, 40, 0, 0, 0, 25, 0, 0, 0, 40, 0}},
{"Drunk", {0, 65, 0, -40, 0, 55, 0, -33, 0, 66, 0, -50, 0, 44, 0, -40}},
{"Rushing", {0, -20, -10, -20, -10, -20, -10, -20, -10, 0, -10, -20, -10, -20, -10, -20}},
{"Dragging", {10, 25, 10, 20, 10, 25, 10, 20, 10, 25, 10, 20, 10, 25, 10, 20}},
{"Random 1", {0, 15, -10, 25, 0, 5, -20, 10, 0, 20, -5, 15, 0, 10, -15, 5}},
{"Random 2", {5, -5, 10, -10, 15, -15, 20, -20, 5, -5, 10, -10, 15, -15, 20, -20}},
{"Polyrhythm", {0, 0, 0, 30, 0, 0, 30, 0, 0, 30, 0, 0, 30, 0, 0, 0}},

};

// Helper function to get shuffle template name for OLED display
inline const char* getShuffleTemplateName(uint8_t index) {
    if (index >= NUM_SHUFFLE_TEMPLATES) {
        return "Invalid";
    }
    return shuffleTemplates[index].name;
}
#endif // SHUFFLE_TEMPLATES_H
