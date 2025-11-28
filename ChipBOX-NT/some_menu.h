#ifndef SOME_MENU_H
#define SOME_MENU_H

#include <Adafruit_SSD1306.h>
#include <Adafruit_MPR121.h>
#include "font4x6.h"
#include "key_event.h"
#include "src_config.h"
#include "extra_func.h"
#include "windowGUI.h"
#include "esp_spiffs.h"
#include "BASFNT.h"
#include "wifi.h"

extern Adafruit_MPR121 touchPad0;
extern Adafruit_MPR121 touchPad1;

extern bool enableTouchPad;
extern bool touchPadMode;
extern uint32_t SMP_RATE;

extern Adafruit_SSD1306 display;

void displayKeyboard(const char *title, char *targetStr, uint8_t maxLen) {
    touchPadMode = true;
    enbOctvAdjust = false;
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    uint8_t curserTick = 0;
    uint8_t charPos = strlen(targetStr);
    bool keyboardStat[16] = {0};
    char charOfst = 'A';
    bool curserStat = false;
    for (;;) {
        display.clearDisplay();
        display.setFont(&font4x6);
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf(title);
        display.setTextColor(1);
        // display.setCursor(0, 12);
        display.setFont(NULL);
        display.drawRect(0, 14, 128, 11, 1);
        display.setCursor(2, 16);
        int16_t len = strlen(targetStr);
        if (len > 20) {
            display.printf("%.20s", targetStr + len - 20);
        } else {
            display.printf("%s", targetStr);
        }
        display.print(curserStat ? '_' : ' ');
        display.drawFastHLine(0, 43, 128, 1);
        display.drawFastHLine(0, 53, 128, 1);
        display.drawFastHLine(0, 63, 128, 1);
        for (uint8_t i = 0; i < 8; i++) {
            display.drawFastVLine(i * 16, 44, 19, 1);
        }
        display.drawFastVLine(127, 44, 19, 1);
        for (uint8_t c = 0; c < 8; c++) {
            display.setTextColor(1);
            display.setCursor((c * 16) + 6, 55);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, 1);
                display.setTextColor(0);
            }
            display.printf("%c", c + charOfst);
        }
        for (uint8_t c = 8; c < 16; c++) {
            display.setTextColor(1);
            display.setCursor(((c - 8) * 16) + 6, 45);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, 1);
                display.setTextColor(0);
            }
            display.printf("%c", c + charOfst);
        }
        display.display();
        curserTick++;
        if (curserTick > 127) {
            curserTick = 0;
            curserStat = !curserStat;
        }
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OCTU) {
                    charOfst += 16;
                    printf("CHAR: %d\n", charOfst);
                } else if (optionKeyEvent.num == KEY_OCTD) {
                    charOfst -= 16;
                    printf("CHAR: %d\n", charOfst);
                } else if (optionKeyEvent.num == KEY_S) {
                    if (charPos > 0) {
                        charPos--;
                        targetStr[charPos] = '\0';
                    }
                } else if (optionKeyEvent.num == KEY_OK) {
                    break;
                }
            }
        }
        if (readTouchPadKeyEvent == pdTRUE) {
            if (touchPadEvent.status == KEY_ATTACK) {
                targetStr[charPos] = touchPadEvent.num + charOfst;
                charPos++;
                if (charPos > maxLen) {
                    charPos--;
                }
                targetStr[charPos] = '\0';
                keyboardStat[touchPadEvent.num] = true;
            } else if (touchPadEvent.status == KEY_RELEASE) {
                keyboardStat[touchPadEvent.num] = false;
            }
        }
        vTaskDelay(2);
    }
    display.setFont(&font4x6);
}

int aMenu(const char *title, const char *menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void)) {
    display.setFont(&font4x6);
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    int8_t menuPos = 0;
    int8_t pageStart = 0;
    const uint8_t itemsPerPage = 5;

    for (;;) {
        // 检查是否需要更新页面
        if (menuPos < pageStart) {
            pageStart = menuPos;
        } else if (menuPos >= pageStart + itemsPerPage) {
            pageStart = menuPos - itemsPerPage + 1;
        }

        display.clearDisplay();
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf(title);
        display.setTextColor(1);

        uint8_t pageEnd = (pageStart + itemsPerPage > maxMenuPos + 1) ? maxMenuPos + 1 : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            uint8_t displayIndex = i - pageStart;
            if (i == menuPos) {
                display.drawRect(0, 10 + (displayIndex * 10), 128, 10, 1);
            }
            display.setCursor(2, 12 + (displayIndex * 10));
            if (i >= maxMenuPos) {
                display.printf("EXIT");
            } else {
                display.printf(menuStr[i]);
            }
        }
        display.display();

        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            if (optionKeyEvent.num == KEY_OK) {
                if (menuPos >= maxMenuPos) return -1;
                if ((menuFunc != nullptr) && (menuFunc[menuPos] != nullptr)) {
                    menuFunc[menuPos]();
                } else {
                    return menuPos;
                }
            } else if (optionKeyEvent.num == KEY_BACK) {
                return -1;
            } else if (optionKeyEvent.num == KEY_UP) {
                menuPos--;
                if (menuPos < 0) {
                    menuPos = maxMenuPos;
                    pageStart = (maxMenuPos / itemsPerPage) * itemsPerPage;
                }
            } else if (optionKeyEvent.num == KEY_DOWN) {
                menuPos++;
                if (menuPos > maxMenuPos) {
                    menuPos = 0;
                    pageStart = 0;
                }
            }
        }
        readTouchPadKeyEvent;
        vTaskDelay(2);
    }
}

void numSetMenu(const char *title, int *num, int min, int max, uint8_t count, void (*refresh_func)(int)) {
    float numCount = 128.0f / max;
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    display.setFont(&font4x6);
    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf(title);
        display.setTextColor(1);
        display.setCursor(0, 20);
        display.printf("%d/%d", *num, max);
        display.drawRect(0, 30, 128, 8, 1);
        display.fillRect(0, 31, *num * numCount, 7, 1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK || optionKeyEvent.num == KEY_BACK) {
                    if (refresh_func != NULL)
                        refresh_func(*num);
                    return;
                } else if (optionKeyEvent.num == KEY_P) {
                    *num += count;
                    if (*num > max) *num = max;
                    UpKeyStat = true;
                } else if (optionKeyEvent.num == KEY_S) {
                    *num -= count;
                    if (*num < min) *num = min;
                    DownKeyStat = true;
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                    if (refresh_func != NULL)
                        refresh_func(*num);
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                    if (refresh_func != NULL)
                        refresh_func(*num);
                }
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            *num += count;
            if (*num > max) *num = max;
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            *num -= count;
            if (*num < min) *num = min;
        }
        readTouchPadKeyEvent;
        vTaskDelay(2);
    }
}

void numSetMenuFloat(const char *title, float *num, float min, float max, float count, void (*refresh_func)(float)) {
    float numCount = 128 / max;
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    display.setFont(&font4x6);
    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf(title);
        display.setTextColor(1);
        display.setCursor(0, 20);
        display.printf("%f/%f", *num, max);
        display.drawRect(0, 30, 128, 8, 1);
        display.fillRect(0, 31, *num * numCount, 7, 1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK || optionKeyEvent.num == KEY_BACK) {
                    if (refresh_func != NULL)
                        refresh_func(*num);
                    return;
                } else if (optionKeyEvent.num == KEY_P) {
                    *num += count;
                    if (*num > max) *num = max;
                    UpKeyStat = true;
                } else if (optionKeyEvent.num == KEY_S) {
                    *num -= count;
                    if (*num < min) *num = min;
                    DownKeyStat = true;
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                    if (refresh_func != NULL)
                        refresh_func(*num);
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                    if (refresh_func != NULL)
                        refresh_func(*num);
                }
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            *num += count;
            if (*num > max) *num = max;
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            *num -= count;
            if (*num < min) *num = min;
        }
        readTouchPadKeyEvent;
        vTaskDelay(2);
    }
}

void reverb_setting() {
    key_event_t optionKeyEvent;
    reverb_config_t reverb_config_tmp = chipbox_config.reverb_config;
    const uint8_t maxMenuPos = 5;
    const char *menuStr[maxMenuPos] = {"DELAY LINE", "DECAY", "MIX", "ROOMSIZE", "DAMPING"};
    for (;;) {
        int ret = aMenu("REVERB CONFIG", menuStr, maxMenuPos, NULL);
        switch (ret) {
        case 0:
            numSetMenu("DELAY LINE", &reverb_config_tmp.numDelays, 0, 8, 1, NULL);
            break;

        case 1:
            numSetMenuFloat("DECAY", &reverb_config_tmp.decay, 0.0f, 1.0f, 0.01f, NULL);
            break;

        case 2:
            numSetMenuFloat("MIX", &reverb_config_tmp.mix, 0.0f, 1.0f, 0.01f, NULL);
            break;

        case 3:
            numSetMenuFloat("ROOMSIZE", &reverb_config_tmp.roomSize, 0.0f, 1.0f, 0.01f, NULL);
            break;

        case 4:
            numSetMenuFloat("DAMPING", &reverb_config_tmp.damping, 0.0f, 1.0f, 0.01f, NULL);
            break;
        }
        if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    chipbox_config.reverb_config = reverb_config_tmp;
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void delay_setting() {
    key_event_t optionKeyEvent;
    delay_config_t delay_config_tmp = chipbox_config.delay_config;
    const uint8_t maxMenuPos = 4;
    const char *menuStr[maxMenuPos] = {"LENGTH", "DECAY", "DRYMIX", "WETMIX"};
    for (;;) {
        int ret = aMenu("DELAY CONFIG", menuStr, maxMenuPos, NULL);
        switch (ret) {
        case 0:
            numSetMenu("LENGTH MS", &delay_config_tmp.Length, 0, 20000, 1, NULL);
            break;

        case 1:
            numSetMenuFloat("DECAY", &delay_config_tmp.decayRate, 0.0f, 1.0f, 0.01f, NULL);
            break;

        case 2:
            numSetMenuFloat("DRYMIX", &delay_config_tmp.dryMix, 0.0f, 1.0f, 0.01f, NULL);
            break;

        case 3:
            numSetMenuFloat("WETMIX", &delay_config_tmp.wetMix, 0.0f, 1.0f, 0.01f, NULL);
            break;
        }
        if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    chipbox_config.delay_config = delay_config_tmp;
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void refresh_delay_len(int len) {
    audio_delay.initialize(chipbox_config.delay_config);
}

void set_delay_decay(float decay) {
    audio_delay.set_decay(decay);
}

void set_delay_wet(float wet) {
    audio_delay.set_decay(wet);
}

void set_delay_dry(float dry) {
    audio_delay.set_decay(dry);
}

void delay_setting_read_time() {
    key_event_t optionKeyEvent;
    delay_config_t *delay_config_tmp = &chipbox_config.delay_config;
    const uint8_t maxMenuPos = 4;
    const char *menuStr[maxMenuPos] = {"LENGTH", "DECAY", "DRYMIX", "WETMIX"};
    for (;;) {
        int ret = aMenu("DELAY CONFIG", menuStr, maxMenuPos, NULL);
        switch (ret) {
        case 0:
            numSetMenu("LENGTH MS", &delay_config_tmp->Length, 0, 20000, 1, refresh_delay_len);
            break;

        case 1:
            numSetMenuFloat("DECAY", &delay_config_tmp->decayRate, 0.0f, 1.0f, 0.01f, set_delay_decay);
            break;

        case 2:
            numSetMenuFloat("DRYMIX", &delay_config_tmp->dryMix, 0.0f, 1.0f, 0.01f, set_delay_dry);
            break;

        case 3:
            numSetMenuFloat("WETMIX", &delay_config_tmp->wetMix, 0.0f, 1.0f, 0.01f, set_delay_wet);
            break;
        }
        if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void effect_setting() {
    key_event_t optionKeyEvent;
    const uint8_t maxMenuPos = 5;
    const char *menuStr[maxMenuPos] = {"DELAY", "REVERB", "LOWPASS", "HIGHPASS", "FM"};
    for (;;) {
        int ret = aMenu("EFFECT", menuStr, maxMenuPos, NULL);
        switch (ret) {
        case 0:
            delay_setting();
            break;

        case 1:
            reverb_setting();
            break;

        case 2:
            numSetMenu("LOWPASS HZ", &chipbox_config.nesLowPass, 0, 44100, 16, NULL);
            break;

        case 3:
            numSetMenu("HIGHPASS HZ", &chipbox_config.nesHighPass, 0, 44100, 16, NULL);
            break;

        case 4:
            numSetMenuFloat("FM UNITE", &chipbox_config.fmLevel, 0.0f, 1.0f, 0.001f, NULL);
            break;
        }
        if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void set_lowpass_cut(int cut) {
    lowpassfilter.setCutoffFrequency(cut, SMP_RATE);
}

void set_highpass_cut(int cut) {
    highpassfilter.setCutoffFrequency(cut, SMP_RATE);
}

void effect_setting_real_time() {
    key_event_t optionKeyEvent;
    const uint8_t maxMenuPos = 4;
    const char *menuStr[maxMenuPos] = {"DELAY", "LOWPASS", "HIGHPASS", "FM"};
    for (;;) {
        int ret = aMenu("EFFECT", menuStr, maxMenuPos, NULL);
        switch (ret) {
        case 0:
            delay_setting_read_time();
            break;

        case 1:
            numSetMenu("LOWPASS HZ", &chipbox_config.nesLowPass, 0, 44100, 16, set_lowpass_cut);
            break;

        case 2:
            numSetMenu("HIGHPASS HZ", &chipbox_config.nesHighPass, 0, 44100, 16, set_highpass_cut);
            break;

        case 3:
            numSetMenuFloat("FM UNITE", &chipbox_config.fmLevel, 0.0f, 1.0f, 0.001f, NULL);
            break;
        }
        if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void led_setting() {
    const uint8_t maxMenuPos = 4;
    const char *menuStr[maxMenuPos] = {"ENABLE LED", "ENABLE FADE IN", "MAX BRIG", "DELAY MS"};
    for (;;) {
        int ret = aMenu("LED", menuStr, maxMenuPos, NULL);
        if (ret == 0) {
            int8_t boolret = boolWindowMenu("ENABLE LED", "ENABLE LED?     ");
            if (boolret == 1) chipbox_config.enbLED = true;
            if (boolret == 0) chipbox_config.enbLED = false;
        } else if (ret == 1) {
            int8_t boolret = boolWindowMenu("ENABLE FADE IN", "ENABLE FADE IN?    ");
            if (boolret == 1) chipbox_config.enbLEDFadeIn = true;
            if (boolret == 0) chipbox_config.enbLEDFadeIn = false;
        } else if (ret == 2) {
            numSetMenu("MAX BRIG", &chipbox_config.LEDMaxBrig, 0, 255, 1, NULL);
        } else if (ret == 3) {
            numSetMenu("DELAY MS", &chipbox_config.LEDDelayMS, 0, 255, 1, NULL);
        } else if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void touch_pad_test() {
    enableTouchPad = false;
    key_event_t optionKeyEvent;
    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf("TOUCHPAD TEST");
        display.setTextColor(1);
        display.setCursor(0, 12);
        readOptionKeyEvent;
        if (optionKeyEvent.status == KEY_ATTACK && optionKeyEvent.num == KEY_BACK) {
            break;
        }
        display.printf("GROUP0:\n");
        display.setFont(&font3x5);
        for (uint8_t i = 4; i < 12; i++) {
            uint16_t baseLine = touchPad0.baselineData(i);
            display.printf("%d ", baseLine);
        }
        display.printf("\n");
        for (uint8_t i = 4; i < 12; i++) {
            uint16_t originVar = touchPad0.filteredData(i);
            display.printf("%d ", originVar);
        }
        display.setFont(&font4x6);
        display.setCursor(0, 36);
        display.printf("GROUP1:\n");
        display.setFont(&font3x5);
        for (uint8_t i = 4; i < 12; i++) {
            uint16_t baseLine = touchPad1.baselineData(i);
            display.printf("%d ", baseLine);
        }
        display.printf("\n");
        for (uint8_t i = 4; i < 12; i++) {
            uint16_t originVar = touchPad1.filteredData(i);
            display.printf("%d ", originVar);
        }
        display.setFont(&font4x6);
        display.display();
        vTaskDelay(2);
    }
    enableTouchPad = true;
}

void touchPad_setting() {
    const uint8_t maxMenuPos = 3;
    const char *menuStr[maxMenuPos] = {"ATK THRES", "RLS THRES", "TOUCH PAD TEST"};
    for (;;) {
        int ret = aMenu("TOUCHPAD", menuStr, maxMenuPos, NULL);
        if (ret == 0) {
            numSetMenu("ATK THRES", &chipbox_config.mpr121AtkScen, 0, 255, 1, NULL);
        } else if (ret == 1) {
            numSetMenu("RLS THRES", &chipbox_config.mpr121RlsScen, 0, 255, 1, NULL);
        } else if (ret == 2) {
            touch_pad_test();
        } else if (ret == -1) {
            break;
        }
        vTaskDelay(1);
    }
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void key_test_setting() {
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    bool menuStat = false;
    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 8, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.printf("KEY TEST");
        display.setTextColor(1);
        display.setCursor(0, 12);
        readOptionKeyEvent;
        readTouchPadKeyEvent;
        if (optionKeyEvent.num == KEY_MENU) {
            if (optionKeyEvent.status == KEY_ATTACK)
                menuStat = true;
            else
                menuStat = false;
        }
        if (menuStat) {
            if (optionKeyEvent.num == KEY_BACK) {
                break;
            }
        }
        display.printf("KEY:\n%d %s", optionKeyEvent.num, optionKeyEvent.status == KEY_ATTACK ? "KEY_ATTCK" : "KEY_RELEASE");
        display.setCursor(0, 36);
        display.printf("TOUCHPAD:\n%d %s", touchPadEvent.num, touchPadEvent.status == KEY_ATTACK ? "KEY_ATTCK" : "KEY_RELEASE");
        display.display();
        vTaskDelay(2);
    }
}

void smp_rate_setting() {
    numSetMenu("SAMP RATE", &chipbox_config.SMP_RATE, 0, 192000, 100, NULL);
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void buffer_size_setting() {
    numSetMenu("BUFF SIZE", &chipbox_config.BUFF_SIZE, 0, 65535, SMP_RATE * 0.001f, NULL);
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void vib_deep_setting() {
    numSetMenu("VIB DEEP HZ", &chipbox_config.vibDeep, 0, 64, 1, NULL);
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void sound_option() {
    const uint8_t maxMenuPos = 2;
    const char *menuStr[maxMenuPos] = {"SAMP RATE", "BUFF SIZE"};
    void (*menuFunc[])(void) = {smp_rate_setting, buffer_size_setting};
    aMenu("SOUND OPTION", menuStr, maxMenuPos, menuFunc);
}

void setContrast(int contrast);

void oled_brig_setting() {
    numSetMenu("OLED BRIG", &chipbox_config.oled_brig, 0, 255, 1, setContrast);
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
}

void oled_invert_setting() {
    int8_t ret = boolWindowMenu("ENABLE INVERT", "ENABLE INVERT? ");
    if (ret == 1) chipbox_config.displayInvert = true;
    if (ret == 0) chipbox_config.displayInvert = false;
    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
    display.setFont(&font4x6);
    if (chipbox_config.displayInvert)
        display.ssd1306_command(SSD1306_INVERTDISPLAY);
    else
        display.ssd1306_command(SSD1306_NORMALDISPLAY);
}

void hardware_setting() {
    const uint8_t maxMenuPos = 6;
    const char *menuStr[maxMenuPos] = {"LED", "TOUCHPAD", "SOUND OPTION", "OLED BRIG", "OLED INVER", "KEY_TEST"};
    void (*menuFunc[])(void) = {led_setting, touchPad_setting, sound_option, oled_brig_setting, oled_invert_setting, key_test_setting};
    aMenu("HARDWARE", menuStr, maxMenuPos, menuFunc);
}

void about_setting() {
    key_event_t optionKeyEvent;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("ABOUT");
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.printf("CHIPBOX VERSION %d\nDEBUGGING: %d\nHARDWARE: 2.1\n%s\n%s\nCAT BIT STUDIO", CHIPBOX_VERSION, ENB_DEBUGGING, __DATE__, __TIME__);
    display.display();
    for (;;) {
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            break;
        }
        vTaskDelay(64);
    }
}

void sound_eng_setting() {
    const uint8_t maxMenuPos = 1;
    const char *menuStr[maxMenuPos] = {"VIB DEEP"};
    void (*menuFunc[])(void) = {vib_deep_setting};
    aMenu("SOUND ENG", menuStr, maxMenuPos, menuFunc);
}

void reset_config() {
    int8_t ret = boolWindowMenu("RESET CONFIG", "ARE YOU SURE!?  ");
    display.setFont(&font4x6);
    if (ret == 1) {
        show_reboot_info();
        remove(CHIPBOX_CONFIG_PATH);
        esp_restart();
    }
}

void format_spiffs() {
    display.setFont(&font4x6);
    display.clearDisplay();
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("FORMAT SPIFFS");
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.printf("FORMATTING...\nPLEASE WAIT");
    display.display();
    esp_spiffs_format("spiffs");
    show_reboot_info();
    save_instrument("/spiffs/inst_b_0.inst", NULL);
    esp_restart();
}

void format_spiffs_setting() {
    int8_t ret = boolWindowMenu("FORMAT SPIFFS", "ARE YOU SURE!?  ");
    display.setFont(&font4x6);
    if (ret == 1) {
        int8_t ret1 = boolWindowMenu("ARE YOU SURE!?", "THIS WILL CLEAR ALL INST!");
        display.setFont(&font4x6);
        if (ret1 == 1) {
            format_spiffs();
        }
    }
}

void reset_setting() {
    const uint8_t maxMenuPos = 2;
    const char *menuStr[maxMenuPos] = {"RESET CONFIG", "FORMAT SPIFFS"};
    void (*menuFunc[])(void) = {reset_config, format_spiffs_setting};
    aMenu("RESET OPTION", menuStr, maxMenuPos, menuFunc);
}

void wifi_setting() {
    key_event_t optionKeyEvent;
    display.setFont(&font4x6);
    display.clearDisplay();
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("WIFI");
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.printf("INIT_STA...\n");
    display.display();
    wifi_init_sta("zeus1", "010939luwei");
    wifi_global_event_t event;
    while (1) {
        // 从队列中接收事件
        if (xQueueReceive(wifi_event_queue, &event, portMAX_DELAY) == pdPASS) {
            if (event.event_base == WIFI_EVENT) {
                if (event.event_id == WIFI_EVENT_STA_START) {
                    display.printf("Wi-Fi started, connecting...\n");
                    esp_wifi_connect();
                } else if (event.event_id == WIFI_EVENT_STA_DISCONNECTED) {
                    display.printf("Wi-Fi disconnected, reconnecting...\n");
                    esp_wifi_connect();
                }
            } else if (event.event_base == IP_EVENT && event.event_id == IP_EVENT_STA_GOT_IP) {
                ip_event_got_ip_t *got_ip_event = (ip_event_got_ip_t *)event.event_data;
                s_ip_addr = got_ip_event->ip_info.ip;
                display.printf("IP: " IPSTR "\n", IP2STR(&s_ip_addr));
                break;
            }
        }
        vTaskDelay(2);
    }
    display.printf("DELAY 10s\n");
    display.display();
    vTaskDelay(10000);
    display.printf("DESCONNECT...\n");
    display.display();
    wifi_disconnect();
    display.printf("END\n");
    display.display();
    for (;;) {
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            break;
        }
        vTaskDelay(64);
    }
}

void ota_setting() {

}

void test_displayKeyboard() {
    char testStr[32] = {0};
    key_event_t optionKeyEvent;
    displayKeyboard("TEST KEYBOARD", testStr, 31);
    printf("Return! testStr:\n%s\n", testStr);
    display.clearDisplay();
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("TEST KEYBOARD");
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.setTextWrap(true);
    display.printf("Return! testStr:\n%s\n", testStr);
    display.setTextWrap(false);
    display.display();
    for (;;) {
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            break;
        }
        vTaskDelay(64);
    }
}

void BASFNT_test() {
    display.setFont(&font4x6);
    key_event_t optionKeyEvent;
    display.clearDisplay();
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("TEST FONT");
    display.setTextColor(1);
    display.setFont(&BASFNT);
    display.setCursor(0, 12);
    display.setTextWrap(true);
    display.printf("HELLO,WORLD!\nhello,world!\nlibchara-dev");
    display.setTextWrap(false);
    display.display();
    display.setFont(&font4x6);
    for (;;) {
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            break;
        }
        vTaskDelay(64);
    }
}

void debug_menu() {
    touchPadMode = true;
    const uint8_t maxMenuPos = 9;
    const char *menuStr[maxMenuPos] = {"EFFECT", "HARDWARE", "RESET OPTION", "SOUND ENG", "TEST DISPLAYKEYBOARD", "FONT", "WIFI", "OTA", "ABOUT"};
    void (*menuFunc[])(void) = {effect_setting, hardware_setting, reset_setting, sound_eng_setting, test_displayKeyboard, BASFNT_test, wifi_setting, ota_setting, about_setting};
    aMenu("DEBUG MENU", menuStr, maxMenuPos, menuFunc);
}

#endif