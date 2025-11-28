#ifndef COLOR_TABLE_H
#define COLOR_TABLE_H

#include <stdint.h>

#define LED_ROOT_BRIG 190
#define LED_UNROOT_BRIG 115

const uint16_t LEDH_table[12] = {240, 276, 320, 17, 29, 50, 80, 0, 167, 203, 0};
const uint8_t LEDS_table[12] = {180, 180, 180, 180, 180, 180, 180, 180, 0, 180, 180, 0};

typedef enum {
    LED_MODE_AUTO,
    LED_MODE_SCALE
} led_mode_t;

#endif