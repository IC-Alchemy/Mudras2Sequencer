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
{"Lil' Swing (51%)", {0, 5, 0, 6, 0, 5, 0, 6, 0, 5, 0, 6, 0, 4, 0, 6}},
{"Lil' Swing (53%)", {0, 10, 0, 10, 0, 10, 0, 10, 0, 10, 0, 10, 0, 10, 0, 10}},
{"Lil' Swing (54%)", {0, 15, 0, 13, 0, 15, 0, 13, 0, 15, 0, 13, 0, 15, 0, 13}},

// Added new swing templates
{"Swing (55%)", {0, 17, 0, 15, 0, 17, 0, 15, 0, 17, 0, 15, 0, 17, 0, 15}},
{"Swing (56%)", {0, 19, 0, 18, 0, 19, 0, 20, 0, 19, 0, 20, 0, 23, 0, 20}},
{"Swing (57%)", {0, 25, 0, 20, 0, 21, 0, 26, 0, 21, 0, 22, 0, 23, 0, 23}},
{"Swing (60%)", {0, 30, 0, 25, 0, 30, 0, 25, 0, 25, 0, 25, 0, 30, 0, 25}},

{"Big Swang (60%)", {0, 37, 0, 30, 0, 37, 0, 33, 0, 37, 0, 33, 0, 37, 0, 30}},
{"Big Swang (62%)", {0, 45, 0, 40, 0, 39, 0, 40, 0, 37, 0, 45, 0, 40, 0, 40}},
{"Humanize 1", {0, -4, 0, 3, 0, -2, 0, 3, 0, -3, 0, 3, 0, 2, 0, -3}},
{"Humanize 2", {0, -1, -2, 2, 1, 2, 0, -2, -1, 0, 1, 1, 2, 2, 3, 1}},

{"Hip-Hop", {0, 30, 0, 22, 0, 30, 0, 22, 0, 30, 0, 22, 0, 30, 0, 22}},
{"Funk Groove", {0, 35, 0, 20, 0, 30, 0, 20, 0, 35, 0, 20, 0, 30, 0, 20}},
{"Latin Tumbao", {0, 0, 25, 0, 0, 0, 40, 0, 0, 0, 25, 0, 0, 0, 40, 0}},
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
