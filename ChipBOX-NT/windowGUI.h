#ifndef WINDOWGUI_H
#define WINDOWGUI_H

#include "key_event.h"
#include <Adafruit_SSD1306.h>
#include <stdint.h>
#include <string.h>

extern Adafruit_SSD1306 display;

#define drawMidRect(w, h, color) \
    display.drawRect(64 - ((w) >> 1), 32 - ((h) >> 1), (64 + ((w) >> 1)) - (64 - ((w) >> 1)), (32 + ((h) >> 1)) - (32 - ((h) >> 1)), (color))

#define drawMidRectOfst(w, h, color, ofstX, ofstY) \
    display.drawRect(64 - ((w) >> 1) + (ofstX), 32 - ((h) >> 1) + (ofstY), \
                   (w), (h), (color))

#define fillMidRect(w, h, color) \
    display.fillRect(64 - ((w) >> 1), 32 - ((h) >> 1), (64 + ((w) >> 1)) - (64 - ((w) >> 1)), (32 + ((h) >> 1)) - (32 - ((h) >> 1)), (color))

#define fillMidRectOfst(w, h, color, ofstX, ofstY) \
    display.fillRect(64 - ((w) >> 1) + (ofstX), 32 - ((h) >> 1) + (ofstY), \
                   (w), (h), (color))

#define setMidCusr(w, h, ofst) \
    display.setCursor(64 - ((w) >> 1) + (ofst), 32 - ((h) >> 1) + (ofst))

#define setMidCusrOfst(w, h, ofstX, ofstY) \
    display.setCursor(64 - ((w) >> 1) + (ofstX), 32 - ((h) >> 1) + (ofstY))

#define drawMidHlineOfst(x, y, w, ofstX, ofstY) \
    display.drawFastHLine(64 - ((x) >> 1) + (ofstX), 32 - ((y) >> 1) + (ofstY), w, 1)

#define getMidAdrX(x, y) 64 - ((x) >> 1)
#define getMidAdrY(x, y) 32 - ((y) >> 1)

int8_t boolWindowMenu(const char *title, const char *profile) {
    uint8_t windowsLen = (strlen(profile) * 4) + 4;
    key_event_t optionKeyEvent;
    int8_t stat = true;
    uint8_t midStartX = getMidAdrX(windowsLen, 28);
    uint8_t midStartY = getMidAdrY(windowsLen, 28);
    for (uint8_t y = 0; y < 64; y++) {
        for (uint8_t x = 0; x < 128; x ++) {
            if ((x + y) & 1) {
                display.drawPixel(x, y, 0);
            }
        }
    }
    display.display();
    for (;;) {
        fillMidRect(windowsLen, 28, 0);
        drawMidRect(windowsLen, 28, 1);
        display.setFont(&font3x5);
        setMidCusrOfst(windowsLen, 28, 2, 2);
        display.printf(title);
        setMidCusrOfst(windowsLen, 28, 2, 10);
        display.printf(profile);
        drawMidHlineOfst(windowsLen, 28, windowsLen, 0, 8);
        // drawMidHlineOfst(windowsLen, 28, windowsLen, 0, 19);
        display.setCursor(midStartX+2, midStartY+21);
        if (stat == 1) {
            display.setTextColor(0);
            display.fillRect(midStartX+1, midStartY+20, 14, 7, 1);
        }
        display.printf("YES");
        display.setTextColor(1);
        display.setCursor(midStartX+19, midStartY+21);
        if (stat == 0) {
            display.setTextColor(0);
            display.fillRect(midStartX+16, midStartY+20, 15, 7, 1);
        }
        display.printf("NO");
        display.setTextColor(1);
        display.drawFastHLine(midStartX, midStartY+19, windowsLen, 1);
        display.drawFastVLine(midStartX+15, midStartY+20, 7, 1);
        display.drawFastVLine(midStartX+30, midStartY+20, 7, 1);
        vTaskDelay(2);
        display.display();
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            if (optionKeyEvent.num == KEY_L) {
                stat++;
                if (stat > 1) stat = 1;
            } else if (optionKeyEvent.num == KEY_R) {
                stat--;
                if (stat < 0) stat = 0;
            } else if (optionKeyEvent.num == KEY_OK) {
                return stat;
            } else if (optionKeyEvent.num == KEY_BACK) {
                return -1;
            }
        }
    }
    return -2;
}

#endif