#ifndef EXTRA_FUNC_H
#define EXTRA_FUNC_H

#include <Adafruit_SSD1306.h>
#include <stdio.h>
#include "basic_dsp.h"

typedef enum {
    PAGE_MAIN,
    PAGE_INST,
    PAGE_SCALE,
    PAGE_DEBUG
} page_pos_t;

extern Adafruit_SSD1306 display;
extern int32_t BPM_tick_max;
extern uint8_t global_BPM;
AudioDelay audio_delay;
AudioReverb audio_reverb;
LowPassFilter lowpassfilter;
HighPassFilter highpassfilter;

typedef struct {
    int SMP_RATE = 96000;
    int BUFF_SIZE = 512;
    int8_t global_vol = 8;
    int8_t scale_num = 0;
    int8_t map_root_key = 0;
    bool enableNoteMap = true;
    bool enbLED = true;
    bool enbLEDFadeIn = false;
    int LEDDelayMS = 20;
    int LEDMaxBrig = 10;
    int nesHighPass = 100;
    int nesLowPass = 36000;
    float fmLevel = 0.0f;
    int mpr121AtkScen = 0x09;
    int mpr121RlsScen = 0x06;
    int oled_brig = 255;
    int vibDeep = 8;
    bool displayInvert = false;
    delay_config_t delay_config;
    reverb_config_t reverb_config;
} chipbox_config_t;

chipbox_config_t chipbox_config;

#define LED_STRIP_BLINK_GPIO  11
#define LED_STRIP_LED_NUMBERS 16
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

const uint8_t touch_map[24] = 
    {255, 255, 255, 255,
        11, 3, 10, 2, 9, 1, 0, 8, 15, 7, 14, 6, 5, 13, 4, 12, 255, 255, 255, 255};

void write_chipbox_config(const char* filename, chipbox_config_t *config) {
    FILE *file = fopen(filename, "wb");
    fwrite(config, sizeof(chipbox_config_t), 1, file);
    fclose(file);
}

void read_chipbox_config(const char* filename, chipbox_config_t *config) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        chipbox_config_t new_config;
        write_chipbox_config(filename, &new_config);
        file = fopen(filename, "rb");
    }
    fread(config, sizeof(chipbox_config_t), 1, file);
    fclose(file);
}

void show_reboot_info() {
    display.fillScreen(1);
    display.setFont(NULL);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.print("REBOOT...");
    display.display();
}

const char* esp_reset_reason_to_name(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_UNKNOWN:    return "ESP_RST_UNKNOWN";
        case ESP_RST_POWERON:    return "ESP_RST_POWERON";
        case ESP_RST_EXT:        return "ESP_RST_EXT";
        case ESP_RST_SW:         return "ESP_RST_SW";
        case ESP_RST_PANIC:      return "ESP_RST_PANIC";
        case ESP_RST_INT_WDT:    return "ESP_RST_INT_WDT";
        case ESP_RST_TASK_WDT:   return "ESP_RST_TASK_WDT";
        case ESP_RST_WDT:        return "ESP_RST_WDT";
        case ESP_RST_DEEPSLEEP:  return "ESP_RST_DEEPSLEEP";
        case ESP_RST_BROWNOUT:   return "ESP_RST_BROWNOUT";
        case ESP_RST_SDIO:       return "ESP_RST_SDIO";
        case ESP_RST_USB:        return "ESP_RST_USB";
        case ESP_RST_JTAG:       return "ESP_RST_JTAG";
        case ESP_RST_EFUSE:      return "ESP_RST_EFUSE";
        case ESP_RST_PWR_GLITCH: return "ESP_RST_PWR_GLITCH";
        case ESP_RST_CPU_LOCKUP: return "ESP_RST_CPU_LOCKUP";
        default:                 return "UNKNOWN_REASON";
    }
}

int mapTouchToNote(uint8_t touchNum, const uint8_t note_map[], uint8_t note_map_len, uint8_t note_octave, uint8_t root_note) {
    uint8_t octave_shift = touchNum / note_map_len;
    uint8_t note_index = touchNum % note_map_len;
    uint8_t note = root_note + note_map[note_index] + (note_octave + octave_shift) * 12;

    return note;
}

void drawQSqur(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill) {
    display.drawFastHLine(x+1, y, w-2, 1);
    display.drawFastHLine(x+1, y+h-1, w-2, 1);
    display.drawFastVLine(x, y+1, h-2, 1);
    display.drawFastVLine(x+w-1, y+1, h-2, 1);
    if (fill)
        display.fillRect(x+1, y+1, w-2, h-2, 1);
}

const char *noteStr[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

bool enbOctvAdjust = true;

#endif