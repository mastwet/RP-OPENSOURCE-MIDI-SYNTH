// 如果我很有钱该多好
#include "src_config.h"
#include <Arduino.h>

uint32_t SMP_RATE;
uint16_t BUFF_SIZE;

#include <esp_system.h>
#include <esp_core_dump.h>
#if ENB_MEM_DEBUG
#include "debug_memory.h"
#endif
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_Keypad.h>
#include "TM1650.h"
#include "icon_data.h"
#include "icon_data_word.h"
#include "inst_sel_icon.h"
#include "icon_data_inst_edit.h"
#include "wave_icon.h"
#include "led_drive/led_strip.h"
#include "driver/i2s_std.h"
#include "fast_sin.h"
#include "basic_dsp.h"
#include "SerialTerminal.h"
#include "font5x7.h"
#include "font3x5.h"
#include "font4x6.h"
#include "esp_spiffs.h"
#include "listfile.h"
#include "wave_table.h"
#include "midi_note_freq.h"
#include "nes_noise.h"
#include "instrument.h"
#include "gamma_led.h"
#include "minilzo/minilzo.h"
#include "extra_func.h"
#include "key_event.h"
#include "note.h"
#include "windowGUI.h"
#include "cube.h"
#include "note_map_type.h"
#include "some_menu.h"
#include "scale_mode_ui.h"
#include "kick_sample.h"
#include "creak_sample.h"
#include "clap_sample.h"
#include "hihat_sample.h"
#include "color_table.h"

#define ROWS 4 // rows
#define COLS 3 // columns

typedef enum {
    INST_BASS,
    INST_KEY,
    INST_PLUCK,
    INST_ARP,
    INST_OTHER
} inst_type_t;

const float poly_limiter[MAX_POLYPHONY+1] = {1, 1, 0.9f, 0.76f, 0.62f, 0.51f, 0.45f, 0.41f, 0.38f};

void init_notes() {
    for (int i = 0; i < MAX_POLYPHONY; i++) {
        notes[i].status = NOTE_OFF;
    }
}

int16_t *originBuffer;
int16_t *outputBuffer;
uint8_t global_BPM = 60;
int32_t BPM_tick = 0;
int32_t BPM_tick_max = 0;
uint32_t global_clock_tick = 0;

uint8_t actv_chan = 0;

uint16_t inst_crossover;

instruments_t now_inst_data;
inst_type_t now_inst_type = INST_ARP;
int8_t now_inst_num;

// inst_x_num[0] = built-in, [1] = user, [2] = like
uint8_t inst_a_num[3];
uint8_t inst_b_num[3];
uint8_t inst_p_num[3];
uint8_t inst_k_num[3];
uint8_t inst_u_num[3];

led_mode_t led_mode = LED_MODE_AUTO;

int8_t led_brig[LED_STRIP_LED_NUMBERS] = {0};

bool led_stat[LED_STRIP_LED_NUMBERS] = {false};

// bool led_disable_auto[LED_STRIP_LED_NUMBERS] = {false};

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags = {
            .invert_out = false
        } // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags = {
            .with_dma = false
        } // DMA feature is available on ESP target like ESP32-S3
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    // ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

led_strip_handle_t led_strip;

TaskHandle_t SOUND_ENG;
TaskHandle_t LED_TASK;

Adafruit_MPR121 touchPad0;
Adafruit_MPR121 touchPad1;
/*
const uint8_t touch_map[24] = 
    {255, 255, 255, 255,
        3, 11, 2, 10, 1, 9, 8, 0, 7, 15, 6, 14, 13, 5, 12, 4, 255, 255, 255, 255};
*/

const uint8_t led_map[16] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};

int8_t scale_num = 0;
int8_t map_root_key = 0;
bool enableNoteMap = true;

uint8_t key_map[ROWS][COLS] = {
  {KEY_L,KEY_OK,KEY_MENU},
  {KEY_UP,KEY_S,KEY_NAVI},
  {KEY_R,KEY_BACK,KEY_OCTD},
  {KEY_DOWN,KEY_P,KEY_OCTU}
};
uint8_t rowPins[ROWS] = {47, 18, 45, 46};
uint8_t colPins[COLS] = {38, 39, 48};

Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(key_map), rowPins, colPins, ROWS, COLS);

int8_t ProgressBarStatus = 0;

Adafruit_SSD1306 display(128, 64, &SPI, 7, 15, 6, 10000000);

float wave_t_c;
float wave_time[MAX_POLYPHONY] = {0};
uint8_t clock_speed_f = 60;
uint16_t clock_speed_c;

float note_freq;
int8_t note_octave = 4;
page_pos_t page_pos = PAGE_MAIN;

bool enableTouchPad = true;
bool touchPadMode = false;
bool touchPadAutoLED = true;

inline int clamp(int value, int min, int max) {
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}

void refresh_inst_count() {
    printf("inst_count:\n");

    count_matching_files("/spiffs/", "inst", &inst_a_num[0], &inst_b_num[0], &inst_p_num[0], &inst_k_num[0], &inst_u_num[0]);
    printf("BUILT-IN INST:\n");
    printf("ARP: %d\nBASS: %d\nPLUCK: %d\nKEY: %d\nOTHER: %d\n", inst_a_num[0], inst_b_num[0], inst_p_num[0], inst_k_num[0], inst_u_num[0]);

    count_matching_files("/spiffs/", "user", &inst_a_num[1], &inst_b_num[1], &inst_p_num[1], &inst_k_num[1], &inst_u_num[1]);
    printf("\nUSER INST:\n");
    printf("ARP: %d\nBASS: %d\nPLUCK: %d\nKEY: %d\nOTHER: %d\n", inst_a_num[1], inst_b_num[1], inst_p_num[1], inst_k_num[1], inst_u_num[1]);
}

#define OVERSAMPLING_FACTOR 2

inline int16_t make_sound(uint8_t channel, float freq, uint8_t vol, wave_type_t wave_type) {
    if (vol == 0 || notes[channel].status == NOTE_OFF) return notes[channel].last_sample;
    if (vol > 15) vol = 15;

    if (wave_type == WAVE_NOSE) {
        return nes_noise_make(notes[channel].note_rel & 15, (notes[channel].wave - 7) & 1, SMP_RATE) * vol;
    } else {
        int32_t sum = 0;
        float wave_time_local = wave_time[channel];
        float wave_step = wave_t_c * freq / OVERSAMPLING_FACTOR;

        for (uint8_t i = 0; i < OVERSAMPLING_FACTOR; i++) {
            uint8_t index = (uint8_t)wave_time_local & 31;
            sum += wave_table[wave_type][index] * vol;
            wave_time_local += wave_step;

            if (wave_time_local >= 32) {
                wave_time_local -= 32;
            }
        }

        wave_time[channel] = wave_time_local;

        int16_t result = (sum >> 3) << 6;
        notes[channel].last_sample = result;
        return result;
    }
}

inline int16_t make_sample(uint8_t channel, float freq, uint8_t vol, void *samp, uint32_t len) {
    if (samples[channel].status == NOTE_OFF || vol == 0) {
        return 0;
    }
    samples[channel].time += freq / SMP_RATE;
    if (samples[channel].time >= len) {
        samples[channel].status = NOTE_OFF;
        // samples[channel].vol = 0;
    }
    int32_t ret = ((int32_t)((int16_t*)samp)[(uint32_t)roundf(samples[channel].time)] * vol) >> 11;
    return (int16_t)ret;
}

void byte_to_binary(byte byte, char *binary_str) {
    for (int i = 7; i >= 0; i--) {
        binary_str[7 - i] = (byte & (1 << i)) ? '1' : '0';
    }
    binary_str[8] = '\0';
}

void refresh_crossover() {
    if (now_inst_data.frq) {
        inst_crossover = roundf(SMP_RATE / (float)now_inst_data.frq);
        printf("Set Crossover to %d\n", inst_crossover);
    } else {
        printf("WARNING: INST FRQ IS 0, USE 60Hz\n");
        inst_crossover = SMP_RATE / 60;
    }
}

void reset_all_note() {
    for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
        notes[i].status = NOTE_OFF;
        notes[i].note = 0;
        notes[i].freq = 0;
        notes[i].vol = 0;
    }
}

void input(void *arg) {
    const char *TAG = "INPUT";
    Wire1.begin(8, 9);
    Wire1.setClock(200000);
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    keypad.begin();
    touchPad0.begin(0x5A, &Wire1);
    touchPad1.begin(0x5B, &Wire1);
    touchPad0.setThresholds(chipbox_config.mpr121AtkScen, chipbox_config.mpr121RlsScen);
    touchPad1.setThresholds(chipbox_config.mpr121AtkScen, chipbox_config.mpr121RlsScen);
    // touchPad0.setThresholds(0x25, 0x24);
    // touchPad1.setThresholds(0x25, 0x24);
    uint16_t lasttouched0;
    uint16_t currtouched0;
    uint16_t lasttouched1;
    uint16_t currtouched1;
    for (;;) {
        keypad.tick();
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            optionKeyEvent.num = e.bit.KEY;
            if(e.bit.EVENT == KEY_JUST_PRESSED) optionKeyEvent.status = KEY_ATTACK;
            else if(e.bit.EVENT == KEY_JUST_RELEASED) optionKeyEvent.status = KEY_RELEASE;
            if (enbOctvAdjust) {
                if (optionKeyEvent.num == KEY_OCTD && optionKeyEvent.status == KEY_ATTACK) {
                    note_octave--;
                    if (note_octave < 0) note_octave = 0;
                    printf("OCT %d\n", note_octave);
                    reset_all_note();
                }
                if (optionKeyEvent.num == KEY_OCTU && optionKeyEvent.status == KEY_ATTACK) {
                    note_octave++;
                    if (note_octave > 9) note_octave = 9;
                    printf("OCT %d\n", note_octave);
                    reset_all_note();
                }
            }
            if (xQueueSend(xOptionKeyQueue, &optionKeyEvent, portMAX_DELAY) != pdPASS) {
                ESP_LOGW(TAG, "WARNING: OPTIONKEY QUEUE LOSS A EVENT!\n");
            } else {
                ESP_LOGI(TAG, "INFO: OPTIONKEY SUCCESSFULLY SENT A EVENT. NUM=%d STATUS=%s\n", optionKeyEvent.num, optionKeyEvent.status == KEY_ATTACK ? "ATTACK" : "RELEASE");
            }
        }
        if (enableTouchPad) {
            currtouched0 = touchPad0.touched();
            for (uint8_t i = 4; i < 12; i++) {
                // it if *is* touched and *wasnt* touched before, alert!
                if ((currtouched0 & _BV(i)) && !(lasttouched0 & _BV(i)) ) {
                    if (touchPadMode) {
                        touchPadEvent.num = touch_map[i];
                        touchPadEvent.status = KEY_ATTACK;
                    } else {
                        touchPadEvent.num = touch_map[i];
                        if (enableNoteMap)
                            touchPadEvent.num = mapTouchToNote(touchPadEvent.num, scale_data[scale_num].note_map, scale_data[scale_num].note_map_len, note_octave, map_root_key);
                        else
                            touchPadEvent.num = touchPadEvent.num + (note_octave * 12);

                        touchPadEvent.status = KEY_ATTACK;
                    }
                    if (xQueueSend(touchPadMode ? xTouchPadQueue : xNoteQueue, &touchPadEvent, portMAX_DELAY) == pdPASS) {
                        printf("TOUCHPAD %d ATTACK\n", touchPadEvent.num);
                    }
                    if (touchPadAutoLED) {
                        led_stat[touch_map[i]] = true;
                    }
                }
                if (!(currtouched0 & _BV(i)) && (lasttouched0 & _BV(i)) ) {
                    if (touchPadMode) {
                        touchPadEvent.num = touch_map[i];
                        touchPadEvent.status = KEY_RELEASE;
                    } else {
                        touchPadEvent.num = touch_map[i];
                        if (enableNoteMap)
                            touchPadEvent.num = mapTouchToNote(touchPadEvent.num, scale_data[scale_num].note_map, scale_data[scale_num].note_map_len, note_octave, map_root_key);
                        else
                            touchPadEvent.num = touchPadEvent.num + (note_octave * 12);

                        touchPadEvent.status = KEY_RELEASE;
                        led_stat[touch_map[i]] = false;
                    }
                    if (xQueueSend(touchPadMode ? xTouchPadQueue : xNoteQueue, &touchPadEvent, portMAX_DELAY) == pdPASS) {
                        printf("TOUCHPAD %d RELEASE\n", touchPadEvent.num);
                    }
                    led_stat[touch_map[i]] = false;
                }
            }
            lasttouched0 = currtouched0;
            currtouched1 = touchPad1.touched();
            for (uint8_t i = 4; i < 12; i++) {
                if ((currtouched1 & _BV(i)) && !(lasttouched1 & _BV(i)) ) {
                    if (touchPadMode) {
                        touchPadEvent.num = touch_map[i+8];
                        touchPadEvent.status = KEY_ATTACK;
                    } else {
                        touchPadEvent.num = touch_map[i+8];
                        if (enableNoteMap)
                            touchPadEvent.num = mapTouchToNote(touchPadEvent.num, scale_data[scale_num].note_map, scale_data[scale_num].note_map_len, note_octave, map_root_key);
                        else
                            touchPadEvent.num = touchPadEvent.num + (note_octave * 12);

                        touchPadEvent.status = KEY_ATTACK;
                        led_stat[touch_map[i+8]] = true;
                    }
                    if (xQueueSend(touchPadMode ? xTouchPadQueue : xNoteQueue, &touchPadEvent, portMAX_DELAY) == pdPASS) {
                        printf("TOUCHPAD %d ATTACK\n", touchPadEvent.num);
                    }
                    if (touchPadAutoLED) {
                        led_stat[touch_map[i+8]] = true;
                    }
                }
                if (!(currtouched1 & _BV(i)) && (lasttouched1 & _BV(i)) ) {
                    if (touchPadMode) {
                        touchPadEvent.num = touch_map[i+8];
                        touchPadEvent.status = KEY_RELEASE;
                    } else {
                        touchPadEvent.num = touch_map[i+8];
                        if (enableNoteMap)
                            touchPadEvent.num = mapTouchToNote(touchPadEvent.num, scale_data[scale_num].note_map, scale_data[scale_num].note_map_len, note_octave, map_root_key);
                        else
                            touchPadEvent.num = touchPadEvent.num + (note_octave * 12);

                        touchPadEvent.status = KEY_RELEASE;
                        led_stat[touch_map[i+8]] = false;
                    }
                    if (xQueueSend(touchPadMode ? xTouchPadQueue : xNoteQueue, &touchPadEvent, portMAX_DELAY) == pdPASS) {
                        printf("TOUCHPAD %d RELEASE\n", touchPadEvent.num);
                    }
                    led_stat[touch_map[i+8]] = false;
                }
            }
            lasttouched1 = currtouched1;
        }
        vTaskDelay(1);
    }
}


void inst_editor_entry(int argc, const char* argv[]) {
    #if ENB_INST_EDITOR
    vTaskPrioritySet(NULL, 4);
    instruments_t instrument = {0};
    char filename[100] = "";

    printf("ChipBOX Instrument Editor V0.6 (ESP32 Edition)\nby libchara-dev\nBuild Date: %s %s\nInternal tools, please do not share!\n\n", __DATE__, __TIME__);

    interactive_menu(&instrument, filename);

    vTaskPrioritySet(NULL, 3);
    #else
    printf("Instrument editor is disabled\nto use the instrument editor, enable “ENB_INST_EDITOR”\n");
    #endif
}

void copyFile(const char *sourceFile, const char *destFile) {
    FILE *source, *dest;
    char buffer[1024];
    size_t bytesRead;

    source = fopen(sourceFile, "rb");
    if (source == NULL) {
        perror("Failed to open source file");
        exit(EXIT_FAILURE);
    }

    dest = fopen(destFile, "wb");
    if (dest == NULL) {
        fclose(source);
        perror("Failed to open destination file");
        exit(EXIT_FAILURE);
    }

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytesRead, dest);
    }

    fclose(source);
    fclose(dest);

    printf("File copied successfully from %s to %s\n", sourceFile, destFile);
}

void restartCmd(int argc, const char* argv[]) {
    printf("Rebooting...\n");
    ESP.restart();
}

void getVersion(int argc, const char* argv[]) {
    printf("ChipBOX %s\nlibchara-dev%sBuild data: %s %s\nEnable -O3\n%s", ENB_DEBUGGING ? "DEBUGER" : "Beta",
        ENB_DEBUGGING ? "\nBuilding with ArduinoESP32 (IDF5.1.4)\n" : "\n", __DATE__, __TIME__,
            ENB_DEBUGGING ? "Enable debugging\n" : "Debugging is disabled\nto use debugging, enable “ENB_DEBUGGING”\n");
}

#if ENB_DEBUGGING
void key_input_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("Usage: key_input <KEY_L/KEY_R/KEY_UP/KEY_DOWN/KEY_OK>\n");
        return;
    }
    key_event_t optionKeyEvent;
    if (strcmp(argv[1], "KEY_L") == 0)
        optionKeyEvent.num = KEY_L;
    else if (strcmp(argv[1], "KEY_R") == 0)
        optionKeyEvent.num = KEY_R;
    else if (strcmp(argv[1], "KEY_UP") == 0)
        optionKeyEvent.num = KEY_UP;
    else if (strcmp(argv[1], "KEY_DOWN") == 0)
        optionKeyEvent.num = KEY_DOWN;
    else if (strcmp(argv[1], "KEY_OK") == 0)
        optionKeyEvent.num = KEY_OK;
    else {
        printf("Usage: key_input <KEY_L/KEY_R/KEY_UP/KEY_DOWN/KEY_OK>\n");
        return;
    }
    optionKeyEvent.status = KEY_ATTACK;
    if (xQueueSend(xOptionKeyQueue, &optionKeyEvent, portMAX_DELAY) != pdPASS) {
        printf("WARNING: OPTIONKEY QUEUE LOSS A EVENT!\n");
    } else {
        printf("INFO: OPTIONKEY SUCCESSFULLY SENT A EVENT. NUM=%d STATUS=%s\n", optionKeyEvent.num, optionKeyEvent.status == KEY_ATTACK ? "ATTACK" : "RELEASE");
    }
}

void test_cmd(int argc, const char* argv[]) {
    Serial.print("Test command executed with args: ");
    for (int i = 1; i < argc; i++) {
        Serial.print(argv[i]);
        Serial.print(" ");
    }
    Serial.println();
}

void print_cmd(int argc, const char* argv[]) {
    if (argc > 1) printf("%s\n", argv[1]);
}

void jmp_cmd(int argc, const char* argv[]) {
    if (argc == 1) {printf("jmp: jmp <addrs>\n");return;}
    void (*func)() = (void(*)())strtol(argv[1], NULL, 0);
    printf("JUMP TO %p\n", func);
    func();
}

void getmem_cmd(int argc, const char* argv[]) {
    if (argc < 3) {printf("getmem: getmem <addrs> <len>\n");return;}
    uint32_t start = strtol(argv[1], NULL, 0);
    uint32_t len = strtol(argv[2], NULL, 0);
    byte *p = (byte*)start;
    uint32_t end = (uint32_t)p + len;
    uint16_t count = len / 16;
    printf("      ADRS    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F     0123456789ABCEDF\n");
    printf("------------------------------------------------------------------------------------\n");
    for (uint8_t i = 0; i < count; i++) {
        printf("%10p    ", p);
        byte tmp[16];
        for (uint8_t j = 0; j < 16; j++) {
            tmp[j] = *p;
            p++;
        }
        for (uint8_t idx = 0; idx < 16; idx++) {
            printf(tmp[idx] < 0x10 ? "0%X " : "%X ", tmp[idx]);
        }
        printf("    ");
        for (uint8_t idx = 0; idx < 16; idx++) {
            printf(tmp[idx] == NULL ? "." : "%c", iscntrl(tmp[idx]) ? ' ' : tmp[idx]);
        }
        printf("\n");
    }
}

void setmem_cmd(int argc, const char* argv[]) {
    if (argc < 3) {printf("setmem: setmem <addrs> <val>\n");return;}
    uint32_t addrs = strtol(argv[1], NULL, 0);
    byte val = clamp(strtol(argv[2], NULL, 0), 0, 0xff);
    byte *p = (byte*)addrs;
    printf("WRITEING 0x%X TO %p...\n", val, p);
    *p = val;
    printf("VERIFY: VAL=0x%x, MEM=", val);
    if (*p != val) printf("0x%x\nFAILED\n", *p);
    else printf("0x%x\nSUCCESS\n", *p);
}

void listfile_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("ls: ls <path>\n");return;}
    FileInfo* files = NULL;
    int count = list_directory(argv[1], &files);
    if (count < 0) {
        printf("Failed to list directory.\n");
    }

    printf("Total files and directories: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("%s - %s\n", files[i].name, files[i].is_directory ? "Directory" : "File");
        free(files[i].name);
    }
    free(files);
}

void remove_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("rm: rm <path>\n");return;}
    int ret = remove(argv[1]);
    if (ret) {
        printf("Failed to remove %s\n", argv[1]);
    } else {
        printf("Removed file %s\n", argv[1]);
    }
}

void copy_file_cmd(int argc, const char* argv[]) {
    if (argc < 3) {printf("cp: cp <source> <dest>\n");return;}
    copyFile(argv[1], argv[2]);
}

void load_inst_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("load_inst: load_inst <path>\n");return;}
    vTaskSuspend(SOUND_ENG);
    read_instrument(argv[1], &now_inst_data);
    refresh_crossover();
    vTaskResume(SOUND_ENG);
    print_instrument(&now_inst_data);
}

void get_free_heap_cmd(int argc, const char* argv[]) {
    printf("Free heap size: %ld\n", esp_get_free_heap_size());
}

void get_free_heap_cont_cmd(int argc, const char* argv[]) {
    for (;;) {
        printf("Free heap size: %ld\n", esp_get_free_heap_size());
        vTaskDelay(4);
        if (Serial.available()) {
            Serial.read();
            break;
        }
    }
}

void refresh_inst_count_cmd(int argc, const char* argv[]) {
    refresh_inst_count();
}

void view_heap_status_cmd(int argc, const char* argv[]) {
    #if ENB_MEM_DEBUG
    view_heap_status();
    #else
    printf("Enable “ENB_MEM_DEBUG” to use this command\n");
    #endif
}

void print_inst_cmd(int argc, const char* argv[]) {
    print_instrument(&now_inst_data);
}

void print_scale_info(uint8_t num) {
    printf("SCALE #%d\n", num);
    printf("NAME: %s\nFULLNAME: %s\nPROFILE: %s\n", scale_data[num].name, scale_data[num].full_name, scale_data[num].profile);
    printf("LEN = %d\n", scale_data[num].note_map_len);
    printf("DATA: ");
    for (uint8_t i = 0; i < scale_data[num].note_map_len; i++) {
        printf("%d ", scale_data[num].note_map[i]);
    }
    printf("\n\n");
}

void print_scale_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("Usage: %s <Num> (255 = ALL)\n", argv[0]);return;}
    uint8_t num = strtol(argv[1], NULL, 0);
    if (num == 255) {
        for (uint8_t i = 0; i < SCALE_MAX_ITEM; i++) {
            print_scale_info(i);
        }
    } else {
        print_scale_info(num);
    }
}

void set_scale_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("Usage: %s <Num>\n", argv[0]);return;}
    uint8_t num = strtol(argv[1], NULL, 0);
    if (num >= SCALE_MAX_ITEM) {
        printf("FAILED...\n");
        return;
    }
    scale_num = num;
    printf("SCALE SET TO ");
    print_scale_info(scale_num);
}

void set_root_key_cmd(int argc, const char* argv[]) {
    if (argc < 2) {printf("Usage: %s <root key>\n", argv[0]);return;}
    map_root_key = strtol(argv[1], NULL, 0);
    printf("ROOT KEY SET TO %d\n", map_root_key);
}

#endif

void serial_show_display(int argc, const char* argv[]) {
    vTaskPrioritySet(NULL, 10);
    for (;;) {
        uint8_t *displayBuffer = display.getBuffer();
        Serial.printf("\033[H");
        for (uint8_t y = 0; y < 64; y++) {
            for (uint8_t x = 0; x < 128; x++) {
                uint8_t byte = displayBuffer[(x) + (y / 8) * 128];
                int bit = y % 8;
                if (byte & (1 << bit))
                    Serial.print("██");
                else
                    Serial.print("  ");
            }
            Serial.printf("\r\n");
        }
        vTaskDelay(1);
        if (Serial.available()) {
            Serial.read();
            break;
        }
    }
    vTaskPrioritySet(NULL, 3);
}

void SerialInput(void *arg) {
    SerialTerminal terminal;
    vTaskDelay(256);
    terminal.begin(115200, ENB_DEBUGGING ? "ChipBOX DEBUG" : "ChipBOX");
    terminal.addCommand("reboot", restartCmd);
    terminal.addCommand("version", getVersion);
    terminal.addCommand("inst_editor", inst_editor_entry);
    #if ENB_DEBUGGING
    terminal.addCommand("print_inst", print_inst_cmd);
    terminal.addCommand("key_input", key_input_cmd);
    terminal.addCommand("load_inst", load_inst_cmd);
    terminal.addCommand("test", test_cmd);
    terminal.addCommand("print", print_cmd);
    terminal.addCommand("jmp", jmp_cmd);
    terminal.addCommand("getmem", getmem_cmd);
    terminal.addCommand("setmem", setmem_cmd);
    terminal.addCommand("get_free_heap", get_free_heap_cmd);
    terminal.addCommand("get_free_heap_cont", get_free_heap_cont_cmd);
    terminal.addCommand("get_heap_stat", view_heap_status_cmd);
    terminal.addCommand("print_scale", print_scale_cmd);
    terminal.addCommand("set_scale", set_scale_cmd);
    terminal.addCommand("set_root_key", set_root_key_cmd);
    terminal.addCommand("ls", listfile_cmd);
    terminal.addCommand("cp", copy_file_cmd);
    terminal.addCommand("rm", remove_cmd);
    terminal.addCommand("roic", refresh_inst_count_cmd);
    terminal.addCommand("serial_show", serial_show_display);
    #else
    terminal.addCommand("Debugging is disabled", NULL);
    #endif
    for (;;) {
        terminal.update();
        vTaskDelay(1);
    }
}

void mainPage() {
    key_event_t optionKeyEvent;
    // 定义光标位置
    static int8_t xpos = 0;
    static int8_t ypos = 0;
    uint8_t rebootCount = 0;
    for (;;) {
        display.clearDisplay();
        display.drawBitmap(0, 51, Fill[0], 12, 13, 1);
        display.drawBitmap(12, 51, Intro[0], 29, 13, 1);
        display.drawBitmap(41, 51, Drop[0], 29, 13, 1);
        display.drawBitmap(70, 51, Break[0], 29, 13, 1);
        display.drawBitmap(99, 51, Outro[0], 29, 13, 1);
        display.drawBitmap(4, 10, Volume[xpos == 0 && ypos == 0 ? 1 : 0], 74, 13, 1);
        display.fillRect(14, 11, chipbox_config.global_vol << 2, 11, xpos == 0 && ypos == 0 ? 0 : 1);
        if (xpos == 0 && ypos == 0) {
            display.drawFastVLine(14, 11, 11, 1);
            display.drawFastVLine(77, 11, 11, 1);
        }
        display.drawBitmap(83, 10, Bpm[xpos == 1 && ypos == 0 ? 1 : 0], 42, 13, 1);
        display.drawBitmap(2, 26, Musical[xpos == 0 && ypos == 1 ? 1 : 0], 68, 21, 1);
        display.setTextColor(xpos == 0 && ypos == 1 ? 0 : 1);
        display.setFont(&font5x7);
        display.setCursor(18, 29);
        display.printf("%.7s\n", now_inst_data.name);
        display.setCursor(18, display.getCursorY());
        display.printf("%.5s", now_inst_data.name+7);

        display.setTextColor(xpos == 0 && ypos == 1 ? 1 : 0);
        display.setFont(&font3x5);
        display.setCursor(50, 38);
        display.printf(now_inst_num < 10 ? "0%d" : "%d", now_inst_num);

        switch (now_inst_type)
        {   
        case INST_ARP:
            display.drawBitmap(7, 30, ArpEnglishIcon[xpos == 0 && ypos == 1 ? 1 : 0], 8, 13, 1);
            break;
        
        case INST_BASS:
            display.drawBitmap(7, 30, BassEnglishIcon[xpos == 0 && ypos == 1 ? 1 : 0], 8, 13, 1);
            break;

        case INST_KEY:
            display.drawBitmap(7, 30, KeyEnglishIcon[xpos == 0 && ypos == 1 ? 1 : 0], 8, 13, 1);
            break;

        case INST_PLUCK:
            display.drawBitmap(7, 30, PluckEnglishIcon[xpos == 0 && ypos == 1 ? 1 : 0], 8, 13, 1);
            break;

        case INST_OTHER:
            display.drawBitmap(7, 30, UserEnglishIcon[xpos == 0 && ypos == 1 ? 1 : 0], 8, 13, 1);
            break;
        }

        display.drawBitmap(73, 26, Drum[xpos == 1 && ypos == 1 ? 1 : 0], 21, 21, 1);
        display.drawBitmap(97, 26, Scale[xpos == 2 && ypos == 1 ? 1 : 0], 29, 21, 1);
        ProgressBarStatus = ProgressBarStatus & 7;
        for (uint8_t i = 0; i < 8; i++) {
            if (ProgressBarStatus == i) display.drawBitmap(i * 16, 0, ProgressBar[1], 15, 4, 1);
            else display.drawBitmap(i * 16, 0, ProgressBar[i == 0 || i == 4 ? 2 : 0], 15, 4, 1);
        }

        display.display();

        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            if (optionKeyEvent.num == KEY_L) xpos--;
            else if (optionKeyEvent.num == KEY_R) xpos++;
            else if (optionKeyEvent.num == KEY_UP) ypos--;
            else if (optionKeyEvent.num == KEY_DOWN) ypos++;
            else if (optionKeyEvent.num == KEY_P) {
                if (xpos == 0 && ypos == 0) {
                    chipbox_config.global_vol++;
                    if (chipbox_config.global_vol > 16)
                        chipbox_config.global_vol = 16;
                    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
                } else if (xpos == 2 && ypos == 1) {
                    enableNoteMap = true;
                }
            } else if (optionKeyEvent.num == KEY_S) {
                if (xpos == 0 && ypos == 0) {
                    chipbox_config.global_vol--;
                    if (chipbox_config.global_vol < 0)
                        chipbox_config.global_vol = 0;
                    write_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
                } else if (xpos == 2 && ypos == 1) {
                    enableNoteMap = false;
                }
            } else if (optionKeyEvent.num == KEY_BACK) {
                page_pos = PAGE_DEBUG;
                break;
            } else if (optionKeyEvent.num == KEY_OK) {
                if (ypos == 1) {
                    if (xpos == 0) {
                        page_pos = PAGE_INST;
                        break;
                    } else if (xpos == 2) {
                        page_pos = PAGE_SCALE;
                        break;
                    }
                }
            } else if (optionKeyEvent.num == KEY_MENU) {
                effect_setting_real_time();
            } else if (optionKeyEvent.num == KEY_NAVI) {
                if (xpos == 0 && ypos == 0) {
                    rebootCount++;
                    if (rebootCount > 9) {
                        show_reboot_info();
                        esp_restart();
                    }
                }
            } else {
                rebootCount = 0;
            }
        }
        if (ypos < 0) ypos = 0;
        if (xpos < 0) xpos = 0;
        if (xpos > (ypos == 1 ? 2 : 1)) xpos = ypos ? 2 : 1;
        if (ypos > 1) ypos = 1;

        vTaskDelay(1);
    }
}

void cust_enter_print(uint8_t start_x, const char *text) {
    while(*text) {
        if (*text == '\n') {
            display.println();
            display.setCursor(start_x, display.getCursorY()+1);
        } else
            display.print(*text);
        text++;
    }
}

char inst_to_char(inst_type_t type) {
    switch (type) {
        case INST_ARP: return 'a';
        case INST_BASS: return 'b';
        case INST_KEY: return 'k';
        case INST_PLUCK: return 'p';
        case INST_OTHER: return 'u';
    }
    return 0;
}

#include "env_edit.h"

void inst_select() {
    const char *TAG = "INST_SEL";
    key_event_t optionKeyEvent;
    int8_t inst_user_index;
    char inst_user[5];
    int8_t x_sel_pos = 1;
    int8_t sel_pos = 0;
    int8_t side_pos = 0;
    uint8_t page = sel_pos / 5;
    bool in_user_sel = true;
    char name_buff[6][8];
    char file_name_buff[32];
    inst_type_t inst_type = INST_BASS;
    FILE *file;
    bool refresh_profile;
    bool refresh_page;
    int8_t read_ret;
    display.setFont(&font3x5);
    char prev_inst_filename[32];
    bool canDown;
    uint8_t now_type_num;
    strcpy(prev_inst_filename, now_inst_data.file_name);
    for (;;) {
        display.clearDisplay();

        display.drawBitmap(1, 3, Bass_icon[inst_type == INST_BASS], 18, 11, 1);
        display.drawBitmap(1, 15, Key_icon[inst_type == INST_KEY], 18, 11, 1);
        display.drawBitmap(1, 27, Pluck_icon[inst_type == INST_PLUCK], 18, 11, 1);
        display.drawBitmap(1, 39, Arp_icon[inst_type == INST_ARP], 18, 11, 1);
        display.drawBitmap(1, 51, Other_icon[inst_type == INST_OTHER], 18, 11, 1);

        display.drawRect(59, 2, 67, 39, 1);
        display.fillRect(61, 4, 53, 7, 1);
        display.fillRect(115, 4, 9, 7, 1);
        if (in_user_sel) {
            refresh_profile = true;
            refresh_page = true;
            display.drawBitmap(24, 4, BuiltFolder[x_sel_pos == 1 && inst_user_index == 0], 31, 9, 1);
            display.drawBitmap(24, 15, UserFolder[x_sel_pos == 1 && inst_user_index == 1], 31, 9, 1);
            display.drawBitmap(24, 26, LikeFolder[x_sel_pos == 1 && inst_user_index == 2], 31, 9, 1);
            display.setTextColor(0);
            display.setCursor(64, 5);
            switch (inst_user_index) {
                case 0: display.print("BUILT-IN"); break;
                case 1: display.print("USER"); break;
                case 2: display.print("LIKE"); break;
            }
            display.setCursor(116, 5);
            display.printf("0%d", inst_user_index);
            display.setTextColor(1);
            display.setCursor(63, 12);
            switch (inst_user_index) {
                case 0: cust_enter_print(63, "CHIPBOX'S\nBUILT-IN\nINSTRUMENTS"); break;
                case 1: cust_enter_print(63, "INSTRUMENTS\nYOU CREATED"); break;
                case 2: cust_enter_print(63, "YOUR LIKES\nINSTRUMENTS"); break;
            }
            display.drawBitmap(59, 43, wave_none, 67, 20, 1);
            if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_L) {
                    x_sel_pos--;
                    if (x_sel_pos < 0) x_sel_pos = 0;
                } else
                if (optionKeyEvent.num == KEY_R) {
                    x_sel_pos++;
                    if (x_sel_pos > 1) x_sel_pos = 1;
                } else
                if (optionKeyEvent.num == KEY_UP) {
                    if (x_sel_pos < 1) {
                        int tmp = inst_type;
                        tmp--;
                        if (tmp > INST_OTHER) tmp = INST_OTHER;
                        else if (tmp < 0) tmp = 0;
                        inst_type = (inst_type_t)tmp;
                    } else {
                        inst_user_index--;
                        if (inst_user_index < 0) {
                            inst_user_index = 2;
                        }
                    }
                } else
                if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_sel_pos < 1) {
                        int tmp = inst_type;
                        tmp++;
                        if (tmp > INST_OTHER) tmp = INST_OTHER;
                        else if (tmp < 0) tmp = 0;
                        inst_type = (inst_type_t)tmp;
                    } else {
                        inst_user_index++;
                        if (inst_user_index > 2) {
                            inst_user_index = 0;
                        }
                    }
                } else
                if (optionKeyEvent.num == KEY_OK) {
                    switch (inst_user_index) {
                        case 0: strcpy(inst_user, "inst"); break;
                        case 1: strcpy(inst_user, "user"); break;
                    }
                    sel_pos = 0;
                    in_user_sel = false;
                } else
                if (optionKeyEvent.num == KEY_BACK) {
                    page_pos = PAGE_MAIN;
                    break;
                }
            }
        } else {
            page = sel_pos / 6;
            // printf("PAGE %d\n", page);
            switch (inst_type) {
                case INST_ARP: now_type_num = inst_a_num[inst_user_index]; break;
                case INST_BASS: now_type_num = inst_b_num[inst_user_index]; break;
                case INST_PLUCK: now_type_num = inst_p_num[inst_user_index]; break;
                case INST_KEY: now_type_num = inst_k_num[inst_user_index]; break;
                case INST_OTHER: now_type_num = inst_u_num[inst_user_index]; break;
            }

            if (refresh_profile) {
                refresh_profile = false;
                snprintf(file_name_buff, 32, "/spiffs/%s_%c_%d.inst", inst_user, inst_to_char(inst_type), sel_pos);
                read_ret = read_instrument(file_name_buff, &now_inst_data);
                printf("READ FINISH, FILE NAME: %s\n", now_inst_data.file_name);
                if (!read_ret) refresh_crossover();
            }
            if (refresh_page) {
                refresh_page = false;
                if (now_type_num) {
                    canDown = true;
                    for (uint8_t i = 0; i < 6; i++) {
                        if (((page * 6) + i) >= now_type_num) {canDown = false; break;}
                        snprintf(file_name_buff, 32, "/spiffs/%s_%c_%d.inst", inst_user, inst_to_char(inst_type), (page * 6) + i);
                        file = fopen(file_name_buff, "rb");
                        char CI_char[4];
                        fread(CI_char, sizeof(CI_char), 1, file);
                        if (strcmp(CI_char, "CPI")) {
                            strcpy(name_buff[i], "ERROR");
                            // printf("inst %s error\n", file_name_buff);
                        } else {
                            fread(name_buff[i], 8, 1, file);
                            // name_buff[i][7] = 0;
                            // printf("inst %s name %s\n", file_name_buff, name_buff[i]);
                        }
                        fclose(file);
                    }
                } else {
                    sel_pos = 0;
                    x_sel_pos = 0;
                }
            }
            if (read_ret) {
                if (now_type_num) {
                    display.setTextColor(0);
                    display.setCursor(64, 5);
                    display.print("UNKNOW FILE");
                    display.setCursor(116, 5);
                    display.printf("ER");
                    display.setTextColor(1);
                    display.setCursor(63, 12);
                    display.print("INST READ ERROR");
                } else {
                    display.setTextColor(0);
                    display.setCursor(64, 5);
                    display.print("EMPTY");
                    display.setCursor(116, 5);
                    display.printf("NL");
                    display.setTextColor(1);
                    display.setCursor(63, 12);
                    display.print("THE LIST");
                    display.setCursor(63, 19);
                    display.print("IS EMPTY");
                }
            } else {
                display.setTextColor(0);
                display.setCursor(64, 5);
                display.print(now_inst_data.name);
                display.setCursor(116, 5);
                display.printf(sel_pos < 10 ? "0%d" : "%d", sel_pos);
                display.setTextColor(1);
                display.setCursor(63, 12);
                if (now_inst_data.profile_len && now_inst_data.profile != NULL)
                    cust_enter_print(63, now_inst_data.profile);
                else
                    display.print("PROFILE IS NULL");
            }
            if (now_type_num) {
                for (uint8_t i = 0; i < 6; i++) {
                    if (((page * 6) + i) >= now_type_num) break;
                    // display.setCursor(23, (i*10)+5);
                    // display.print(name_buff[i]);
                    display.setCursor(26, (i*10)+5);
                    if (sel_pos % 6 == i) {
                        drawQSqur(24, (i*10)+4, 31, 7, x_sel_pos == 1);
                        display.setTextColor(!(x_sel_pos == 1));
                    } else {
                        display.setTextColor(1);
                    }
                    display.printf("%.8s", name_buff[i]);
                }
            } else {
                display.setCursor(28, 28);
                display.print("EMPTY");
            }

            if (!now_type_num || read_ret) {
                display.drawBitmap(59, 43, wave_none, 67, 20, 1);
            } else {
                display.drawBitmap(59, 43, wave_icon[now_inst_data.wave][x_sel_pos == 2], 21, 20, 1);
                
                display.drawBitmap(82, 43, TheArpeggio[now_inst_data.enb_arp_env][x_sel_pos == 3 & side_pos == 0], 21, 9, 1);
                display.drawBitmap(82, 54, Vibrato[now_inst_data.enb_lfo_env][x_sel_pos == 3 & side_pos == 1], 21, 9, 1);
                display.drawBitmap(105, 43, DutyCycle[now_inst_data.enb_dty_env][x_sel_pos == 4 & side_pos == 0], 21, 9, 1);
                display.drawBitmap(105, 54, Volume_sel[now_inst_data.enb_vol_env][x_sel_pos == 4 & side_pos == 1], 21, 9, 1);
            }

            if (canDown) {
                display.drawFastHLine(35, 62, 3, 1);
                display.drawPixel(36, 63, 1);
            }
            if (page) {
                display.drawFastHLine(35, 1, 3, 1);
                display.drawPixel(36, 0, 1);
            }
            if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_L) {
                    x_sel_pos--;
                    if (x_sel_pos < 0) x_sel_pos = 0;
                } else
                if (optionKeyEvent.num == KEY_R) {
                    x_sel_pos++;
                    if (!now_type_num) x_sel_pos = 0;
                    if (x_sel_pos > 4) x_sel_pos = 4;
                } else
                if (optionKeyEvent.num == KEY_UP) {
                    if (x_sel_pos > 2) {
                        side_pos++;
                        if (side_pos > 1) side_pos = 0;
                    } else if (x_sel_pos < 1) {
                        int tmp = inst_type;
                        tmp--;
                        if (tmp > INST_OTHER) tmp = INST_OTHER;
                        else if (tmp < 0) tmp = 0;
                        inst_type = (inst_type_t)tmp;
                        sel_pos = 0;
                        refresh_profile = true;
                        refresh_page = true;
                    } else {
                        sel_pos--;
                        if (!(sel_pos % 6)) refresh_page = true;
                        if (sel_pos < 0) {
                            sel_pos = now_type_num - 1;
                            refresh_page = true;
                        }
                        refresh_profile = true;
                    }
                } else
                if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_sel_pos > 2) {
                        side_pos++;
                        if (side_pos > 1) side_pos = 0;
                    } else if (x_sel_pos < 1) {
                        int tmp = inst_type;
                        tmp++;
                        if (tmp > INST_OTHER) tmp = INST_OTHER;
                        else if (tmp < 0) tmp = 0;
                        inst_type = (inst_type_t)tmp;
                        sel_pos = 0;
                        refresh_profile = true;
                        refresh_page = true;
                    } else {
                        sel_pos++;
                        if (!(sel_pos % 6)) refresh_page = true;
                        if (sel_pos >= now_type_num) {
                            sel_pos = 0;
                            refresh_page = true;
                        }
                        refresh_profile = true;
                    }
                } else if (optionKeyEvent.num == KEY_P) {
                    if (x_sel_pos == 2) {
                        int tmp = now_inst_data.wave;
                        tmp++;
                        if (tmp > WAVE_NOSE) tmp = WAVE_NOSE;
                        else if (tmp < 0) tmp = 0;
                        now_inst_data.wave = (wave_type_t)tmp;
                    } else if (x_sel_pos == 3) {
                        if (side_pos == 0) {
                            now_inst_data.enb_arp_env = true;
                        } else if (side_pos == 1) {
                            now_inst_data.enb_lfo_env = true;
                        }
                    } else if (x_sel_pos == 4) {
                        if (side_pos == 0) {
                            now_inst_data.enb_dty_env = true;
                        } else if (side_pos == 1) {
                            now_inst_data.enb_vol_env = true;
                        }
                    }
                    save_instrument(now_inst_data.file_name, &now_inst_data);
                } else if (optionKeyEvent.num == KEY_S) {
                    if (x_sel_pos == 2) {
                        int tmp = now_inst_data.wave;
                        tmp--;
                        if (tmp > WAVE_NOSE) tmp = WAVE_NOSE;
                        else if (tmp < 0) tmp = 0;
                        now_inst_data.wave = (wave_type_t)tmp;
                    } else if (x_sel_pos == 3) {
                        if (side_pos == 0) {
                            now_inst_data.enb_arp_env = false;
                        } else if (side_pos == 1) {
                            now_inst_data.enb_lfo_env = false;
                        }
                    } else if (x_sel_pos == 4) {
                        if (side_pos == 0) {
                            now_inst_data.enb_dty_env = false;
                        } else if (side_pos == 1) {
                            now_inst_data.enb_vol_env = false;
                        }
                    }
                    save_instrument(now_inst_data.file_name, &now_inst_data);
                } else if (optionKeyEvent.num == KEY_OK) {
                    if (x_sel_pos < 2) {
                        if (now_type_num) {
                            page_pos = PAGE_MAIN;
                            now_inst_type = inst_type;
                            now_inst_num = sel_pos;
                            return;
                        }
                    } else if (x_sel_pos == 3) {
                        if (side_pos == 0) {
                            arp_env_edit(&now_inst_data);
                        } else if (side_pos == 1) {
                            lfo_env_edit(&now_inst_data);
                        }
                    } else if (x_sel_pos == 4) {
                        if (side_pos == 0) {
                            dty_env_edit(&now_inst_data);
                        } else if (side_pos == 1) {
                            vol_env_edit(&now_inst_data);
                        }
                    }
                } else
                if (optionKeyEvent.num == KEY_BACK) {
                    in_user_sel = true;
                    ESP_LOGI(TAG, "Use Prev Inst: %s\n", prev_inst_filename);
                    read_instrument(prev_inst_filename, &now_inst_data);
                    refresh_crossover();
                }
            }
        }
        display.display();
        vTaskDelay(1);
    }
}

Texture checkerboardTexture;

void generateCheckerboardTexture(Texture texture) {
    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Alternating pattern to create a checkerboard effect
            if ((x / 4 + y / 4) % 2 == 0) {
                texture[y][x] = 1; // Fill square
            } else {
                texture[y][x] = 0; // Leave square empty
            }
        }
    }
}

#include "calculator.h"

void debug_page() {
    key_event_t optionKeyEvent;
    display.setFont(&font4x6);
    display.setTextColor(1);
    display.setTextSize(0);
    uint8_t mode;
    float cube[8][3] = {
        {-20, -20, -20}, {20, -20, -20}, {-20, 20, -20}, {20, 20, -20},
        {-20, -20, 20}, {20, -20, 20}, {-20, 20, 20}, {20, 20, 20}
    };

    Texture *textures;

    textures = (Texture*)malloc(6 * sizeof(Texture));

    float angle = 0.006f;
    float viewerDistance = 80.0f;
    for (;;) {
        if (mode == 0) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.printf("Actv Chan:\n       %d", actv_chan);
            display.setCursor(0, 16);
            display.printf("Chan Stat:\n");
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                display.printf("%2d ", notes[i].note);
            }
            display.printf("\n");
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                display.printf("%2d ", notes[i].vol);
            }
            display.printf("\n");
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                display.printf("%.1f ", wave_time[i]);
            }
            display.printf("\n");
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                display.printf("%.1f ", notes[i].freq);
            }
            display.display();
        } else if (mode == 1) {
            display.clearDisplay();
            display.setFont(&font3x5);
            for (uint8_t x = 0; x < 128; x++) {
                // display.drawPixel(x, 31 + (audioBuffer[x << 2] >> 8), 1);
                // display.drawPixel(x + 1, 31 + (audioBuffer[(x + 1) << 2] >> 8), 1);
                display.drawLine(x, 31 + (originBuffer[x << 1] >> 8), x+1, 31 + (originBuffer[(x+1) << 1] >> 8), 1);
            }
            display.setCursor(0, 58);
            display.printf("OCT%d -> ", note_octave);
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                if (notes[i].status == NOTE_ON) {
                    display.printf("%s ", noteStr[notes[i].note_rel % 12]);
                }
            }
            display.setFont(&font4x6);
            display.display();
        } else if (mode == 2) {
            display.clearDisplay();
            display.setFont(&font3x5);
            for (uint8_t i = 0; i < 6; i++) {
                memset(textures[i], 0, sizeof(textures[i]));
                for (uint8_t x = 0; x < 32; x++) {
                    textures[i][x][(originBuffer[x << 2] >> 9)+15] = 1;
                }
            }
            rotateCube(cube, angle * 0.6f, angle * 0.8f, angle * 0.7f);
            drawCubeWithTextures(cube, textures, viewerDistance);
            display.setCursor(0, 58);
            display.printf("OCT%d -> ", note_octave);
            for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
                if (notes[i].status == NOTE_ON) {
                    display.printf("%s ", noteStr[notes[i].note_rel % 12]);
                }
            }
            display.setFont(&font4x6);
            display.display();
        }
        if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
            if (optionKeyEvent.num == KEY_BACK) {
                page_pos = PAGE_MAIN;
                break;
            } else if (optionKeyEvent.num == KEY_OK) {
                mode++;
                mode = mode % 3;
            } else if (optionKeyEvent.num == KEY_NAVI) {
                Calculator calc;
                display.setTextWrap(false);
                char input[128];
                memset(input, 0, sizeof(input));
                display.setFont(&font4x6);
                display.clearDisplay();
                display.fillRect(0, 0, 128, 8, 1);
                display.setCursor(1, 1);
                display.setTextColor(0);
                display.printf("CALCULATOR++");
                display.setTextColor(1);
                display.setFont(&font3x5);
                display.setCursor(0, 10);
                display.print("NOT A VERY ADVANCED CALCULATOR\nV0.1\nBY LIBCHARA-DEV\n\nPRESS OK TO START...");
                for (;;) {
                    if (readOptionKeyEvent == pdTRUE && optionKeyEvent.status == KEY_ATTACK) {
                        if (optionKeyEvent.num == KEY_OK) {
                            displayKeyboard("INPUT EXPRESSION", input, 127);
                            display.setFont(&font4x6);
                            display.clearDisplay();
                            display.fillRect(0, 0, 128, 8, 1);
                            display.setCursor(1, 1);
                            display.setTextColor(0);
                            display.printf("CALCULATOR++");
                            display.setTextColor(1);
                            display.setFont(&font3x5);
                            display.setCursor(0, 10);
                            calc.process(input);
                            for (uint8_t i = 0; i < 9; i++) {
                                display.print(calc.get_global_buffer(i));
                            }
                        } else if (optionKeyEvent.num == KEY_BACK) {
                            break;
                        }
                    }
                    display.display();
                    vTaskDelay(1);
                }
                display.setFont(&font4x6);
            }
        }
        vTaskDelay(1);
    }
    free(textures);
}

void GUI(void *arg) {
    for (;;) {
        switch (page_pos)
        {
        case PAGE_MAIN:
            mainPage();
            break;
        
        case PAGE_INST:
            inst_select();
            break;

        case PAGE_SCALE:
            scale_mode_ui();
            break;

        case PAGE_DEBUG:
            debug_page();
            break;
        }
        vTaskDelay(1);
    }
}
/*
void reset_all_tick() {
    global_vol_tick = 0;
    global_arp_tick = 0;
    global_lfo_tick = 0;
    global_dty_tick = 0;
}
*/

void note_trigger(uint8_t note, uint8_t vol) {
    const char *TAG = "NOTE_TRI";
    for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
        if (notes[i].status == NOTE_OFF) {
            notes[i].note = note;
            notes[i].freq = midi_freq_table[notes[i].note];
            notes[i].vol = vol;
            notes[i].wave = now_inst_data.enb_dty_env ? (wave_type_t)now_inst_data.dty_env[0] : now_inst_data.wave;
            notes[i].status = NOTE_ON;
            if (now_inst_data.vol_loop != LOOP_STEP)
                notes[i].vol_tick = 0;

            if (now_inst_data.arp_loop != LOOP_STEP)
                notes[i].arp_tick = 0;

            if (now_inst_data.lfo_loop != LOOP_STEP)
                notes[i].lfo_tick = 0;
            
            if (now_inst_data.dty_loop != LOOP_STEP)
                notes[i].dty_tick = 0;
            notes[i].note_rel = notes[i].note;

            notes[i].master_tick = 0;

            if (now_inst_data.vol_loop == LOOP_SIGL || now_inst_data.vol_loop == LOOP_CYLE) {
                notes[i].vol_micro_tick = 0;
                if (now_inst_data.vol_upend)
                    notes[i].vol_tick = now_inst_data.vol_len-1;
                else
                    notes[i].vol_tick = 0;
            }
            if (now_inst_data.arp_loop == LOOP_SIGL || now_inst_data.arp_loop == LOOP_CYLE) {
                notes[i].arp_micro_tick = 0;
                if (now_inst_data.arp_upend)
                    notes[i].arp_tick = now_inst_data.arp_len-1;
                else
                    notes[i].arp_tick = 0;
            }
            if (now_inst_data.lfo_loop == LOOP_SIGL || now_inst_data.lfo_loop == LOOP_CYLE) {
                notes[i].lfo_micro_tick = 0;
                if (now_inst_data.lfo_upend)
                    notes[i].lfo_tick = now_inst_data.lfo_len-1;
                else
                    notes[i].lfo_tick = 0;
            }
            if (now_inst_data.dty_loop == LOOP_SIGL || now_inst_data.dty_loop == LOOP_CYLE) {
                notes[i].dty_micro_tick = 0;
                if (now_inst_data.dty_upend)
                    notes[i].dty_tick = now_inst_data.dty_len-1;
                else
                    notes[i].dty_tick = 0;
            }

            if (now_inst_data.wave == WAVE_NOSE)
                ESP_LOGI(TAG, "noise_freq_table[%d] = %d\n", notes[i].note & 15, noise_freq_table[1][notes[i].note & 15]);
            else
                ESP_LOGI(TAG, "midi_freq_table[%d] = %f\n", notes[i].note, notes[i].freq);

            wave_time[i] = 0;
            break;
        }
    }
}

void note_release(uint8_t note) {
    for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
        if (notes[i].note == note && notes[i].status == NOTE_ON) {
            notes[i].status = NOTE_OFF;
            notes[i].note = 0;
            notes[i].vol = 0;
            notes[i].freq = 0;
            notes[i].note_rel = 0;
            wave_time[i] = 0;
            if (now_inst_data.vol_loop == LOOP_STEP) {
                if (now_inst_data.vol_upend)
                    notes[i].vol_tick--;
                else
                    notes[i].vol_tick++;
            }
            if (now_inst_data.arp_loop == LOOP_STEP) {
                if (now_inst_data.arp_upend)
                    notes[i].arp_tick--;
                else
                    notes[i].arp_tick++;
            }
            if (now_inst_data.lfo_loop == LOOP_STEP) {
                if (now_inst_data.lfo_upend)
                    notes[i].lfo_tick--;
                else
                    notes[i].lfo_tick++;
            }
            if (now_inst_data.dty_loop == LOOP_STEP) {
                if (now_inst_data.dty_upend)
                    notes[i].dty_tick--;
                else
                    notes[i].dty_tick++;
            }
            break;
        }
    }
}

void refresh_note_status() {
    global_clock_tick++;
    for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
        if (notes[i].status == NOTE_ON) {
            if (now_inst_data.enb_vol_env) {
                if (notes[i].vol_tick >= now_inst_data.vol_len) {
                    if (now_inst_data.vol_loop == LOOP_SIGL)
                        notes[i].vol_tick = now_inst_data.vol_len - 1;
                    else
                        notes[i].vol_tick = 0;
                } else if (notes[i].vol_tick < 0) {
                    if (now_inst_data.vol_loop == LOOP_SIGL)
                        notes[i].vol_tick = 0;
                    else
                        notes[i].vol_tick = now_inst_data.vol_len - 1;
                }
                notes[i].vol = now_inst_data.vol_env[notes[i].vol_tick];
                // printf("note_vol = %d\n", notes[i].vol);
            }

            if (now_inst_data.enb_arp_env) {
                if (notes[i].arp_tick >= now_inst_data.arp_len) {
                    if (now_inst_data.arp_loop == LOOP_SIGL)
                        notes[i].arp_tick = now_inst_data.arp_len - 1;
                    else
                        notes[i].arp_tick = 0;
                } else if (notes[i].arp_tick < 0) {
                    if (now_inst_data.arp_loop == LOOP_SIGL)
                        notes[i].arp_tick = 0;
                    else
                        notes[i].arp_tick = now_inst_data.arp_len - 1;
                }
                notes[i].note_rel = notes[i].note + now_inst_data.arp_env[notes[i].arp_tick];
                notes[i].freq = midi_freq_table[notes[i].note_rel];
            }

            if (now_inst_data.enb_lfo_env) {
                if (notes[i].lfo_tick >= now_inst_data.lfo_len) {
                    if (now_inst_data.lfo_loop == LOOP_SIGL)
                        notes[i].lfo_tick = now_inst_data.lfo_len - 1;
                    else
                        notes[i].lfo_tick = 0;
                } else if (notes[i].lfo_tick < 0) {
                    if (now_inst_data.lfo_loop == LOOP_SIGL)
                        notes[i].lfo_tick = 0;
                    else
                        notes[i].lfo_tick = now_inst_data.lfo_len - 1;
                }
                notes[i].vibSpd = now_inst_data.lfo_env[notes[i].lfo_tick];
            }

            if (now_inst_data.enb_dty_env) {
                if (notes[i].dty_tick >= now_inst_data.dty_len) {
                    if (now_inst_data.dty_loop == LOOP_SIGL)
                        notes[i].dty_tick = now_inst_data.dty_len - 1;
                    else
                        notes[i].dty_tick = 0;
                } else if (notes[i].dty_tick < 0) {
                    if (now_inst_data.dty_loop == LOOP_SIGL)
                        notes[i].dty_tick = 0;
                    else
                        notes[i].dty_tick = now_inst_data.dty_len - 1;
                }
                notes[i].wave = (wave_type_t)now_inst_data.dty_env[notes[i].dty_tick];
            }
        }

        notes[i].master_tick++;
        if (notes[i].master_tick > inst_crossover) {
            notes[i].master_tick = 0;
            notes[i].vol_micro_tick++;
            notes[i].arp_micro_tick++;
            notes[i].lfo_micro_tick++;
            notes[i].dty_micro_tick++;

            if (notes[i].vol_micro_tick >= now_inst_data.vol_rate) {
                notes[i].vol_micro_tick = 0;
                if (now_inst_data.vol_loop != LOOP_STEP) {
                    if (now_inst_data.vol_upend)
                        notes[i].vol_tick--;
                    else
                        notes[i].vol_tick++;
                }
            }

            if (notes[i].arp_micro_tick >= now_inst_data.arp_rate) {
                notes[i].arp_micro_tick = 0;
                if (now_inst_data.arp_loop != LOOP_STEP) {
                    if (now_inst_data.arp_upend)
                        notes[i].arp_tick--;
                    else
                        notes[i].arp_tick++;
                }
            }

            if (notes[i].lfo_micro_tick >= now_inst_data.lfo_rate) {
                notes[i].lfo_micro_tick = 0;
                if (now_inst_data.lfo_loop != LOOP_STEP) {
                    if (now_inst_data.lfo_upend)
                        notes[i].lfo_tick--;
                    else
                        notes[i].lfo_tick++;
                }
                    
            }

            if (notes[i].dty_micro_tick >= now_inst_data.dty_rate) {
                notes[i].dty_micro_tick = 0;
                if (now_inst_data.dty_loop != LOOP_STEP) {
                    if (now_inst_data.dty_upend)
                        notes[i].dty_tick--;
                    else
                        notes[i].dty_tick++;
                }
            }
        }
    }
}

void refresh_sample_status() {
    BPM_tick++;
    if (BPM_tick >= BPM_tick_max) {
        BPM_tick -= BPM_tick_max;
        BPM_tick_max = (60 * SMP_RATE) / (global_BPM * MAX_PRECISION);
        printf("BPM TICK TRI! %d %d\n", BPM_tick, BPM_tick_max);
        samples[0].time = 0;
        samples[0].status = NOTE_ON;
    }
}

void soundEng(void *arg) {
    i2s_chan_handle_t tx_handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SMP_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_42,
            .ws = GPIO_NUM_40,
            .dout = GPIO_NUM_41,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    i2s_channel_init_std_mode(tx_handle, &std_cfg);

    key_event_t noteEvent;
    i2s_channel_enable(tx_handle);
    if (chipbox_config.nesLowPass)
        lowpassfilter.setCutoffFrequency(chipbox_config.nesLowPass, SMP_RATE);
    if (chipbox_config.nesHighPass)
        highpassfilter.setCutoffFrequency(chipbox_config.nesHighPass, SMP_RATE);
    if (chipbox_config.delay_config.Length)
        audio_delay.initialize(chipbox_config.delay_config);

    if (chipbox_config.reverb_config.numDelays)
        audio_reverb.initialize(SMP_RATE, chipbox_config.reverb_config);

    Limiter limit(16380, 16380);
    size_t writed;
    uint16_t buffer_count = 0;
    uint16_t tick_count = 0;
    int16_t fmSource = 0;
    init_notes();
    for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
        samples[i].status = NOTE_ON;
    }
    for (;;) {
        int32_t mixed_sample = 0;
        uint8_t active_notes = 0;

        for (uint8_t i = 0; i < MAX_POLYPHONY; i++) {
            float vibVar = now_inst_data.enb_lfo_env ? (ffast_sin((notes[i].vibX >> 8) * notes[i].vibSpd) * chipbox_config.vibDeep) : 0;
            mixed_sample += make_sound(i, notes[i].freq + fmSource + vibVar, notes[i].vol, notes[i].wave);
            if (notes[i].status == NOTE_ON) {
                active_notes++;
            }
            notes[i].vibX++;
        }
        actv_chan = active_notes;

        mixed_sample *= poly_limiter[active_notes];

        if (chipbox_config.nesLowPass)
            mixed_sample = lowpassfilter.process(mixed_sample);

        if (chipbox_config.nesHighPass)
            mixed_sample = highpassfilter.process(mixed_sample);

        // refresh_sample_status();
        // mixed_sample += make_sample(0, 44100, 255, (void*)creak_pcm, creak_pcm_len >> 1);

        if (chipbox_config.delay_config.Length) {
            mixed_sample = audio_delay.process(mixed_sample);
        }

        if (chipbox_config.reverb_config.numDelays) {
            mixed_sample = audio_reverb.process(mixed_sample);
        }

        if (chipbox_config.fmLevel)
            fmSource = originBuffer[buffer_count-1] * chipbox_config.fmLevel;
        else
            fmSource = 0;

        originBuffer[buffer_count] = mixed_sample;

        buffer_count++;
        tick_count++;

        if (buffer_count > BUFF_SIZE) {
            buffer_count = 0;
            for (uint16_t i = 0; i < BUFF_SIZE; i++) {
                outputBuffer[i] = (originBuffer[i] * chipbox_config.global_vol) >> 4;
            }
            i2s_channel_write(tx_handle, outputBuffer, BUFF_SIZE * sizeof(int16_t), &writed, portMAX_DELAY);
            vTaskDelay(1);
        }

        if (tick_count > clock_speed_c) {
            tick_count = 0;
            while (readNoteEvent == pdTRUE) {
                if (noteEvent.status == KEY_ATTACK) {
                    note_trigger(noteEvent.num, 15);
                }
                if (noteEvent.status == KEY_RELEASE) {
                    note_release(noteEvent.num);
                }
            }
        }
        refresh_note_status();
    }
}

void inst_count_task(void *arg) {
    refresh_inst_count();
    vTaskDelete(NULL);
}

void led_test(void *arg) {
    vTaskDelay(16);
    for (int8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
        if (i > 0)
            led_stat[i-1] = false;
        led_stat[i] = true;
        vTaskDelay(26);
    }
    vTaskDelay(26);
    led_stat[LED_STRIP_LED_NUMBERS-1] = false;
    led_mode = LED_MODE_AUTO;
    vTaskDelete(NULL);
}

void led_task(void *arg) {
    xTaskCreatePinnedToCore(led_test, "LED TEST", 1024, NULL, 1, NULL, 1);

    for (;;) {
        if (led_mode == LED_MODE_AUTO) {
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_strip_set_pixel(led_strip, led_map[i], led_brig[i], led_brig[i], led_brig[i]);

                if (led_stat[i]) {
                    if (chipbox_config.enbLEDFadeIn && led_brig[i] < chipbox_config.LEDMaxBrig) {
                        led_brig[i]++;
                    } else {
                        led_brig[i] = chipbox_config.LEDMaxBrig;
                    }
                } else {
                    if (led_brig[i] > 0) {
                        led_brig[i]--;
                    } else {
                        led_brig[i] = 0;
                    }
                }
            }
        } else if (led_mode == LED_MODE_SCALE) {
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_strip_set_pixel_hsv(led_strip, led_map[i], LEDH_table[note_octave], LEDS_table[note_octave], i == map_root_key ? 32 : 4);
            }
        }
        led_strip_refresh(led_strip);
        vTaskDelay(chipbox_config.LEDDelayMS);
    }
}

void print_reboot_res() {
    esp_reset_reason_t ret = esp_reset_reason();
    if (ret != ESP_RST_POWERON && ret != ESP_RST_SW && ret != ESP_RST_UNKNOWN) {
        led_strip = configure_led();
        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
            led_strip_set_pixel(led_strip, i, 20, 0, 0);
        }
        led_strip_refresh(led_strip);
        keypad.begin();
        for (;;) {
            display.clearDisplay();
            display.setFont(&font4x6);
            display.fillRect(0, 0, 128, 8, 1);
            display.setCursor(1, 1);
            display.setTextColor(0);
            display.printf("ABNORMAL REBOOT");
            display.setTextColor(1);
            display.setCursor(0, 12);
            display.printf("WARNING!\n CHIPBOX HAS HAD AN\n ABNORMAL REBOOT\n BECAUSE:\n %s\nCHIPBOX v%d", esp_reset_reason_to_name(ret), CHIPBOX_VERSION);
            display.display();
            if (keypad.available()) {
                show_reboot_info();
                led_strip_clear(led_strip);
                esp_restart();
            }
            keypad.tick();
            vTaskDelay(64);
        }
    }
}

void setContrast(int contrast) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command((uint8_t)contrast);
}

void setup() {
    SPI.begin(17, -1, 16);
    display.begin(SSD1306_SWITCHCAPVCC);
    display.clearDisplay();
    display.setTextSize(0);
    display.setTextWrap(false);
    display.setTextColor(1);
    display.setCursor(0, 0);
    display.printf("ChipBOX VES\nStarting...\n");
    display.display();
    print_reboot_res();
    wifi_event_queue = xQueueCreate(8, sizeof(wifi_global_event_t));
    xOptionKeyQueue = xQueueCreate(8, sizeof(key_event_t));
    xTouchPadQueue = xQueueCreate(8, sizeof(key_event_t));
    xNoteQueue = xQueueCreate(16, sizeof(key_event_t));
    key_event_t optionKeyEvent;

    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 4,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_conf);
    if (ret == ESP_OK) {
        display.printf("SPIFFS mounted!\n");
    } else {
        format_spiffs();
    }
    display.display();

    read_chipbox_config(CHIPBOX_CONFIG_PATH, &chipbox_config);
    setContrast(chipbox_config.oled_brig);
    BUFF_SIZE = chipbox_config.BUFF_SIZE;
    SMP_RATE = chipbox_config.SMP_RATE;
    enableNoteMap = chipbox_config.enableNoteMap;
    scale_num = chipbox_config.scale_num;
    map_root_key = chipbox_config.map_root_key;
    if (chipbox_config.displayInvert)
        display.ssd1306_command(SSD1306_INVERTDISPLAY);

    clock_speed_c = SMP_RATE / clock_speed_f;
    wave_t_c = 32.0f / SMP_RATE;
    BPM_tick_max = roundf((float)(60 * SMP_RATE) / (float)(global_BPM * MAX_PRECISION));
    inst_crossover = SMP_RATE / 60;

    originBuffer = (int16_t*)malloc(BUFF_SIZE * sizeof(int16_t));
    outputBuffer = (int16_t*)malloc(BUFF_SIZE * sizeof(int16_t));
    memset(originBuffer, 0, BUFF_SIZE * sizeof(int16_t));
    memset(outputBuffer, 0, BUFF_SIZE * sizeof(int16_t));

    read_instrument("/spiffs/inst_b_0.inst", &now_inst_data);
    print_instrument(&now_inst_data);
    xTaskCreate(&inst_count_task, "INST COUNT TASK", 4096, NULL, 4, NULL);
    xTaskCreatePinnedToCore(&input, "KEYPAD", 3000, NULL, 3, NULL, 1);
    xTaskCreate(&SerialInput, "INPUT", 8192, NULL, 3, NULL);
    display.printf("BOOTING...\n");
    display.display();
    vTaskDelay(256);
    display.clearDisplay();
    display.setFont(&font4x6);
    display.fillRect(0, 0, 128, 8, 1);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("CHIPBOX BOOTLOADER");
    display.setTextColor(1);
    display.setCursor(0, 10);
    display.printf("DEBUGGING: %d\nINSTEDITOR: %d\nMEMDEBUG: %d\nVERSION: %d\nREADY..\nOK->BOOT\nMENU->DEBUG MENU\n", ENB_DEBUGGING, ENB_INST_EDITOR, ENB_MEM_DEBUG, CHIPBOX_VERSION);
    display.display();
    if (chipbox_config.enbLED) {
        led_strip = configure_led();
        xTaskCreatePinnedToCore(led_task, "LED", 2048, NULL, 2, &LED_TASK, 1);
    }
    for (;;) {
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.num == KEY_OK) {
                break;
            } else if (optionKeyEvent.num == KEY_MENU) {
                debug_menu();
                show_reboot_info();
                esp_restart();
            }
        }
    }
    display.printf("BOOTING...");
    display.display();
    xTaskCreatePinnedToCore(GUI, "GUI", 20480, NULL, 5, NULL, 0);
    xTaskCreate(&soundEng, "SOUNDENG", 20480, NULL, 6, &SOUND_ENG);
}

void loop() {
    vTaskDelete(NULL);
}