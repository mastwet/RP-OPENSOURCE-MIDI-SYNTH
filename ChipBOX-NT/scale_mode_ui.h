#ifndef SCALE_MODE_UI_H
#define SCLAE_MODE_UI_H

#include <Adafruit_SSD1306.h>
#include "font5x7.h"
#include "key_event.h"
#include "extra_func.h"
#include "note_map_type.h"
#include "scale_icon.h"

extern Adafruit_SSD1306 display;
extern page_pos_t page_pos;
extern int8_t scale_num;
extern int8_t map_root_key;
extern bool enableNoteMap;

extern void reset_all_note();

void scale_mode_ui() {
    key_event_t optionKeyEvent;
    display.setFont(&font5x7);
    display.setTextColor(1);
    display.setTextSize(0);
    bool next = true;
    bool prev = true;
    uint8_t nameLen = strlen(scale_data[scale_num].full_name);
    uint8_t noteStrLen = strlen(noteStr[map_root_key]);
    for (;;) {
        next = true;
        prev = true;
        display.clearDisplay();
        for (uint8_t y = 0; y < 2; y++) {
            for (uint8_t x = 0; x < 3; x++) {
                uint8_t show_tmp = x + (y * 3);
                prev = (scale_num / 6) * 6;
                uint8_t big_show_tmp = ((scale_num / 6) * 6) + show_tmp;
                if (big_show_tmp < SCALE_MAX_ITEM) {
                    display.setCursor(11 + (x * 39), 5 + (y * 12));
                    if ((scale_num % 6) == show_tmp) {
                        display.setTextColor(0);
                        drawQSqur(7 + (x * 39), 3 + (y * 12), 37, 11, 1);
                    }
                    display.print(scale_data[big_show_tmp].name);
                    display.setTextColor(1);
                } else {
                    next = false;
                }
            }
        }
        if (next)
            display.drawBitmap(124, 5, RightPageIcon, 4, 19, 1);
        if (prev)
            display.drawBitmap(0, 5, LeftPageIcon, 4, 19, 1);

        display.drawRect(3, 28, 5 + (6 * nameLen), 11, 1);
        display.setCursor(6, 30);
        display.printf("%s", scale_data[scale_num].full_name);
        display.drawRect(109, 28, 17, 11, 1);
        if (noteStrLen == 1) {
            display.setCursor(115, 30);
        } else if (noteStrLen == 2) {
            display.setCursor(112, 30);
        }
        display.printf("%s", noteStr[map_root_key]);

        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            reset_all_note();
            if (optionKeyEvent.num == KEY_BACK) {
                page_pos = PAGE_MAIN;
                chipbox_config.enableNoteMap = enableNoteMap;
                chipbox_config.scale_num = scale_num;
                chipbox_config.map_root_key = map_root_key;
                write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
                break;
            } else if (optionKeyEvent.num == KEY_R) {
                scale_num++;
                if (scale_num >= SCALE_MAX_ITEM) scale_num = SCALE_MAX_ITEM - 1;
            } else if (optionKeyEvent.num == KEY_L) {
                scale_num--;
                if (scale_num < 0) scale_num = 0;
            } else if (optionKeyEvent.num == KEY_UP) {
                scale_num -= 3;
                if (scale_num < 0) scale_num = 0;
            } else if (optionKeyEvent.num == KEY_DOWN) {
                scale_num += 3;
                if (scale_num >= SCALE_MAX_ITEM) scale_num = SCALE_MAX_ITEM - 1;
            } else if (optionKeyEvent.num == KEY_P) {
                map_root_key++;
                if (map_root_key > 11)
                    map_root_key = 11;
            } else if (optionKeyEvent.num == KEY_S) {
                map_root_key--;
                if (map_root_key < 0)
                    map_root_key = 0;
            }
            reset_all_note();
            nameLen = strlen(scale_data[scale_num].full_name);
            noteStrLen = strlen(noteStr[map_root_key]);
        }
        display.display();
        vTaskDelay(2);
    }
}

#endif