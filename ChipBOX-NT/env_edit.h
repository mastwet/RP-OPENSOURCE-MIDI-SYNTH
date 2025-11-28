#include "instrument.h"
#include <Adafruit_SSD1306.h>
#include "key_event.h"
#include "icon_data_inst_edit.h"
#include "font3x5.h"
#include "font5x7.h"
#include "windowGUI.h"
#include "src_config.h"
#include "note.h"
#include "led_drive/led_strip.h"

extern led_strip_handle_t led_strip;

extern Adafruit_SSD1306 display;

extern TaskHandle_t LED_TASK;

void arp_env_edit(instruments_t *instrument) {
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    int8_t e_pos = 0;
    int8_t e_fct = 0;
    int8_t x_pos = 0;
    int8_t y_pos = 1;
    int8_t spd_pos = 0;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    uint8_t fctTick = 0;
    uint8_t cont;
    bool fnKeyStat = false;
    for (;;) {
        display.clearDisplay();
        display.drawBitmap(2, 1, ArpeggiosTitle, 40, 8, 1);
        uint8_t len = instrument->arp_len;
        display.drawFastHLine(0, 11, 128, 1);
        display.drawFastVLine(109, 0, 11, 1);
        display.drawFastHLine(0, 48, 128, 1);
        display.drawFastVLine(69, 49, 14, 1);
        if (len >= 1) {
            cont = 64;
        }
        if (len >= 2) {
            cont = 32;
        }
        if (len >= 4) {
            cont = 16;
        }
        if (len >= 8) {
            cont = 8;
        }
        if (len >= 16) {
            cont = 4;
        }
        if (len) {
            for (uint8_t index = 0; index < len; index++) {
                uint8_t h = instrument->arp_env[index] + 1;
                bool now_pos = false;
                for (uint8_t n = 0; n < MAX_POLYPHONY; n++) {
                    if (notes[n].status == NOTE_ON && notes[n].arp_tick == index) now_pos = true;
                }
                if (y_pos == 0 && e_pos == index) {
                    now_pos = true;
                    display.drawFastHLine((index * cont) + 1, 13, cont - 1, 1);
                }
                if (h) {
                    if (now_pos)
                        display.fillRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    else
                        display.drawRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    // printf("index=%d cont=%d re=%d rey=%d\n", index, cont, (index * cont) + 1, instrument->arp_env[index] * 2);
                } else {
                    display.drawFastHLine((index * cont) + 1, 45, cont - 1, 1);
                }
            }
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            if (y_pos == 0) {
                display.printf("%3d", instrument->arp_env[e_pos]);
            } else {
                display.printf(" ??");
            }
        } else {
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            display.printf(" ??");
            display.setFont(&font3x5);
            display.setCursor(33, 27);
            display.printf("ENVELOPE IS NULL");
        }
        display.drawBitmap(4, 51, Loop_mode_icon[y_pos == 1 && x_pos == 0][instrument->arp_loop], 11, 11, 1);
        if (instrument->arp_upend) {
            display.drawBitmap(20, 51, Reverse[y_pos == 1 && x_pos == 1], 11, 11, 1);
        } else {
            display.drawBitmap(20, 51, PositiveDirection[y_pos == 1 && x_pos == 1], 11, 11, 1);
        }
        if (instrument->arp_hold) {
            display.drawBitmap(36, 51, Hold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        } else {
            display.drawBitmap(36, 51, UnHold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        }
        if (instrument->arp_mode == ENV_DISC) {
            display.drawBitmap(52, 51, Dispersed[y_pos == 1 && x_pos == 3], 11, 11, 1);
        } else if (instrument->arp_mode == ENV_CONT) {
            display.drawBitmap(52, 51, Continuity[y_pos == 1 && x_pos == 3], 11, 11, 1);
        }
        display.setFont(&font3x5);
        display.setCursor(92, 51);
        display.printf("RATE");
        if (instrument->tick_mode == TICK_FRQ) {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->arp_rate);
        } else {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[1/%d]", instrument->arp_rate);
        }
        display.setTextColor(1);
        display.setCursor(92, 58);
        if (instrument->tick_mode == TICK_FRQ) {
            display.printf("FREQ");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->frq);
        } else {
            display.printf(" BPM");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->bpm);
        }
        display.setTextColor(1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK) {
                    if (y_pos == 1) {
                        if (x_pos == 0) {
                            instrument->arp_loop = (loop_mode_t)(instrument->arp_loop + 1);
                            if (instrument->arp_loop > LOOP_CYLE) instrument->arp_loop = LOOP_SIGL;
                        } else if (x_pos == 1) {
                            instrument->arp_upend = !instrument->arp_upend;
                        } else if (x_pos == 2) {
                            instrument->arp_hold = !instrument->arp_hold;
                        } else if (x_pos == 3) {
                            instrument->arp_mode = (env_mode_t)!instrument->arp_mode;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_BACK) {
                    int8_t ret = boolWindowMenu("SAVE?", "WANT TO SAVE CHANGES?");
                    if (ret == 1) {
                        save_instrument(instrument->file_name, instrument);
                        break;
                    } else if (ret == 0) {
                        read_instrument(instrument->file_name, instrument);
                        break;
                    }
                } else if (optionKeyEvent.num == KEY_P) {
                    if (y_pos == 0) {
                        UpKeyStat = true;
                        instrument->arp_env[e_pos]++;
                        if (instrument->arp_env[e_pos] > 30) instrument->arp_env[e_pos] = 30;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->arp_rate++;
                            if (instrument->arp_rate > 64) instrument->arp_rate = 64;
                        } else if (spd_pos == 1) {
                            instrument->frq++;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_S) {
                    if (y_pos == 0) {
                        DownKeyStat = true;
                        instrument->arp_env[e_pos]--;
                        if (instrument->arp_rate < 1) instrument->arp_rate = 1;
                        if (instrument->arp_env[e_pos] < 0) instrument->arp_env[e_pos] = 0;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->arp_rate--;
                        } else if (spd_pos == 1) {
                            instrument->frq--;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_L) {
                    if (y_pos == 0) {
                        e_pos--;
                        if (e_pos < 0) e_pos = instrument->arp_len-1;
                    } else {
                        x_pos--;
                        if (x_pos < 0) x_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_R) {
                    if (y_pos == 0) {
                        e_pos++;
                        if (e_pos > instrument->arp_len-1) e_pos = 0;
                    } else {
                        x_pos++;
                        if (x_pos > 4) x_pos = 4;
                    }
                } else if (optionKeyEvent.num == KEY_UP) {
                    if (x_pos == 4) {
                        spd_pos = 0;
                    } else {
                        y_pos--;
                        if (y_pos < 0) y_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_pos == 4) {
                        spd_pos = 1;
                    } else {
                        y_pos++;
                        if (y_pos > 1) y_pos = 1;
                    }
                } else if (optionKeyEvent.num == KEY_NAVI) {
                    chipbox_config.enbLEDFadeIn = true;
                    enbOctvAdjust = false;
                    reset_all_note();
                    for (uint8_t i = 0; i < instrument->arp_len - e_fct; i++) {
                        led_stat[i] = true;
                    }
                    touchPadMode = true;
                    touchPadAutoLED = false;
                    fnKeyStat = true;
                } else if (optionKeyEvent.num == KEY_OCTU) {
                    if (!enbOctvAdjust) {
                        e_fct = 15;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->arp_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_OCTD) {
                    if (!enbOctvAdjust) {
                        e_fct = 0;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->arp_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_NAVI) {
                    for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                        led_stat[i] = false;
                    }
                    enbOctvAdjust = true;
                    reset_all_note();
                    fnKeyStat = false;
                    touchPadMode = false;
                    chipbox_config.enbLEDFadeIn = false;
                } else if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                }
            }
        }
        if (readTouchPadKeyEvent == pdTRUE && touchPadEvent.status == KEY_ATTACK) {
            instrument->arp_len = touchPadEvent.num + e_fct + 1;
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_stat[i] = false;
            }
            for (uint8_t i = 0; i < instrument->arp_len - e_fct; i++) {
                led_stat[i] = true;
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->arp_env[e_pos]++;
                if (instrument->arp_env[e_pos] > 30) instrument->arp_env[e_pos] = 30;
            }
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->arp_env[e_pos]--;
                if (instrument->arp_env[e_pos] < 0) instrument->arp_env[e_pos] = 0;
            }
        }
        vTaskDelay(1);
    }
    display.setFont(&font3x5);
}

void dty_env_edit(instruments_t *instrument) {
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    int8_t e_pos = 0;
    int8_t e_fct = 0;
    int8_t x_pos = 0;
    int8_t y_pos = 1;
    int8_t spd_pos = 0;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    uint8_t fctTick = 0;
    uint8_t cont;
    bool fnKeyStat = false;
    for (;;) {
        display.clearDisplay();
        display.drawBitmap(2, 1, DutyCycleTitle, 40, 8, 1);
        uint8_t len = instrument->dty_len;
        display.drawFastHLine(0, 11, 128, 1);
        display.drawFastVLine(109, 0, 11, 1);
        display.drawFastHLine(0, 48, 128, 1);
        display.drawFastVLine(69, 49, 14, 1);
        if (len >= 1) {
            cont = 64;
        }
        if (len >= 2) {
            cont = 32;
        }
        if (len >= 4) {
            cont = 16;
        }
        if (len >= 8) {
            cont = 8;
        }
        if (len >= 16) {
            cont = 4;
        }
        if (len) {
            for (uint8_t index = 0; index < len; index++) {
                uint8_t h = (instrument->dty_env[index] * 8) + 5;
                bool now_pos = false;
                for (uint8_t n = 0; n < MAX_POLYPHONY; n++) {
                    if (notes[n].status == NOTE_ON && notes[n].dty_tick == index) now_pos = true;
                }
                if (y_pos == 0 && e_pos == index) {
                    now_pos = true;
                    display.drawFastHLine((index * cont) + 1, 13, cont - 1, 1);
                }
                if (h) {
                    if (now_pos)
                        display.fillRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    else
                        display.drawRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    // printf("index=%d cont=%d re=%d rey=%d\n", index, cont, (index * cont) + 1, instrument->dty_env[index] * 2);
                } else {
                    display.drawFastHLine((index * cont) + 1, 45, cont - 1, 1);
                }
            }
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            if (y_pos == 0) {
                display.printf("%3d", instrument->dty_env[e_pos]);
            } else {
                display.printf(" ??");
            }
        } else {
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            display.printf(" ??");
            display.setFont(&font3x5);
            display.setCursor(33, 27);
            display.printf("ENVELOPE IS NULL");
        }
        display.drawBitmap(4, 51, Loop_mode_icon[y_pos == 1 && x_pos == 0][instrument->dty_loop], 11, 11, 1);
        if (instrument->dty_upend) {
            display.drawBitmap(20, 51, Reverse[y_pos == 1 && x_pos == 1], 11, 11, 1);
        } else {
            display.drawBitmap(20, 51, PositiveDirection[y_pos == 1 && x_pos == 1], 11, 11, 1);
        }
        if (instrument->dty_hold) {
            display.drawBitmap(36, 51, Hold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        } else {
            display.drawBitmap(36, 51, UnHold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        }
        display.setFont(&font3x5);
        display.setCursor(92, 51);
        display.printf("RATE");
        if (instrument->tick_mode == TICK_FRQ) {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->dty_rate);
        } else {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[1/%d]", instrument->dty_rate);
        }
        display.setTextColor(1);
        display.setCursor(92, 58);
        if (instrument->tick_mode == TICK_FRQ) {
            display.printf("FREQ");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->frq);
        } else {
            display.printf(" BPM");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->bpm);
        }
        display.setTextColor(1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK) {
                    if (y_pos == 1) {
                        if (x_pos == 0) {
                            instrument->dty_loop = (loop_mode_t)(instrument->dty_loop + 1);
                            if (instrument->dty_loop > LOOP_CYLE) instrument->dty_loop = LOOP_SIGL;
                        } else if (x_pos == 1) {
                            instrument->dty_upend = !instrument->dty_upend;
                        } else if (x_pos == 2) {
                            instrument->dty_hold = !instrument->dty_hold;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_BACK) {
                    int8_t ret = boolWindowMenu("SAVE?", "WANT TO SAVE CHANGES?");
                    if (ret == 1) {
                        save_instrument(instrument->file_name, instrument);
                        break;
                    } else if (ret == 0) {
                        read_instrument(instrument->file_name, instrument);
                        break;
                    }
                } else if (optionKeyEvent.num == KEY_P) {
                    if (y_pos == 0) {
                        UpKeyStat = true;
                        instrument->dty_env[e_pos]++;
                        if (instrument->dty_env[e_pos] > 3) instrument->dty_env[e_pos] = 3;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->dty_rate++;
                            if (instrument->dty_rate > 64) instrument->dty_rate = 64;
                        } else if (spd_pos == 1) {
                            instrument->frq++;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_S) {
                    if (y_pos == 0) {
                        DownKeyStat = true;
                        instrument->dty_env[e_pos]--;
                        if (instrument->dty_env[e_pos] < 0) instrument->dty_env[e_pos] = 0;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->dty_rate--;
                            if (instrument->dty_rate < 1) instrument->dty_rate = 1;
                        } else if (spd_pos == 1) {
                            instrument->frq--;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_L) {
                    if (y_pos == 0) {
                        e_pos--;
                        if (e_pos < 0) e_pos = instrument->dty_len-1;
                    } else {
                        x_pos--;
                        if (x_pos < 0) x_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_R) {
                    if (y_pos == 0) {
                        e_pos++;
                        if (e_pos > instrument->dty_len-1) e_pos = 0;
                    } else {
                        x_pos++;
                        if (x_pos > 4) x_pos = 4;
                    }
                } else if (optionKeyEvent.num == KEY_UP) {
                    if (x_pos == 4) {
                        spd_pos = 0;
                    } else {
                        y_pos--;
                        if (y_pos < 0) y_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_pos == 4) {
                        spd_pos = 1;
                    } else {
                        y_pos++;
                        if (y_pos > 1) y_pos = 1;
                    }
                } else if (optionKeyEvent.num == KEY_NAVI) {
                    chipbox_config.enbLEDFadeIn = true;
                    enbOctvAdjust = false;
                    reset_all_note();
                    for (uint8_t i = 0; i < instrument->dty_len - e_fct; i++) {
                        led_stat[i] = true;
                    }
                    touchPadMode = true;
                    touchPadAutoLED = false;
                    fnKeyStat = true;
                } else if (optionKeyEvent.num == KEY_OCTU) {
                    if (!enbOctvAdjust) {
                        e_fct = 15;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->dty_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_OCTD) {
                    if (!enbOctvAdjust) {
                        e_fct = 0;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->dty_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_NAVI) {
                    for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                        led_stat[i] = false;
                    }
                    enbOctvAdjust = true;
                    reset_all_note();
                    fnKeyStat = false;
                    touchPadMode = false;
                    touchPadAutoLED = true;
                    chipbox_config.enbLEDFadeIn = false;
                } else if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                }
            }
        }
        if (readTouchPadKeyEvent == pdTRUE && touchPadEvent.status == KEY_ATTACK) {
            instrument->dty_len = touchPadEvent.num + e_fct + 1;
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_stat[i] = false;
            }
            for (uint8_t i = 0; i < instrument->dty_len - e_fct; i++) {
                led_stat[i] = true;
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->dty_env[e_pos]++;
                if (instrument->dty_env[e_pos] > 3) instrument->dty_env[e_pos] = 3;
            }
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->dty_env[e_pos]--;
                if (instrument->dty_env[e_pos] < 0) instrument->dty_env[e_pos] = 0;
            }
        }
        vTaskDelay(1);
    }
    display.setFont(&font3x5);
}

void vol_env_edit(instruments_t *instrument) {
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    int8_t e_pos = 0;
    int8_t e_fct = 0;
    int8_t x_pos = 0;
    int8_t y_pos = 1;
    int8_t spd_pos = 0;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    uint8_t fctTick = 0;
    uint8_t cont;
    bool fnKeyStat = false;
    for (;;) {
        display.clearDisplay();
        display.drawBitmap(2, 1, VolumeTitle, 40, 8, 1);
        uint8_t len = instrument->vol_len;
        display.drawFastHLine(0, 11, 128, 1);
        display.drawFastVLine(109, 0, 11, 1);
        display.drawFastHLine(0, 48, 128, 1);
        display.drawFastVLine(69, 49, 14, 1);
        if (len >= 1) {
            cont = 64;
        }
        if (len >= 2) {
            cont = 32;
        }
        if (len >= 4) {
            cont = 16;
        }
        if (len >= 8) {
            cont = 8;
        }
        if (len >= 16) {
            cont = 4;
        }
        if (len) {
            for (uint8_t index = 0; index < len; index++) {
                uint8_t h = instrument->vol_env[index] * 2;
                bool now_pos = false;
                for (uint8_t n = 0; n < MAX_POLYPHONY; n++) {
                    if (notes[n].status == NOTE_ON && notes[n].vol_tick == index) now_pos = true;
                }
                if (y_pos == 0 && e_pos == index) {
                    now_pos = true;
                    display.drawFastHLine((index * cont) + 1, 13, cont - 1, 1);
                }
                if (h) {
                    if (now_pos)
                        display.fillRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    else
                        display.drawRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    // printf("index=%d cont=%d re=%d rey=%d\n", index, cont, (index * cont) + 1, instrument->vol_env[index] * 2);
                } else {
                    display.drawFastHLine((index * cont) + 1, 45, cont - 1, 1);
                }
            }
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            if (y_pos == 0) {
                display.printf("%3d", instrument->vol_env[e_pos]);
            } else {
                display.printf(" ??");
            }
        } else {
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            display.printf(" ??");
            display.setFont(&font3x5);
            display.setCursor(33, 27);
            display.printf("ENVELOPE IS NULL");
        }
        display.drawBitmap(4, 51, Loop_mode_icon[y_pos == 1 && x_pos == 0][instrument->vol_loop], 11, 11, 1);
        if (instrument->vol_upend) {
            display.drawBitmap(20, 51, Reverse[y_pos == 1 && x_pos == 1], 11, 11, 1);
        } else {
            display.drawBitmap(20, 51, PositiveDirection[y_pos == 1 && x_pos == 1], 11, 11, 1);
        }
        if (instrument->vol_hold) {
            display.drawBitmap(36, 51, Hold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        } else {
            display.drawBitmap(36, 51, UnHold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        }
        if (instrument->vol_mode == ENV_DISC) {
            display.drawBitmap(52, 51, Dispersed[y_pos == 1 && x_pos == 3], 11, 11, 1);
        } else if (instrument->vol_mode == ENV_CONT) {
            display.drawBitmap(52, 51, Continuity[y_pos == 1 && x_pos == 3], 11, 11, 1);
        }
        display.setFont(&font3x5);
        display.setCursor(92, 51);
        display.printf("RATE");
        if (instrument->tick_mode == TICK_FRQ) {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->vol_rate);
        } else {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[1/%d]", instrument->vol_rate);
        }
        display.setTextColor(1);
        display.setCursor(92, 58);
        if (instrument->tick_mode == TICK_FRQ) {
            display.printf("FREQ");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->frq);
        } else {
            display.printf(" BPM");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->bpm);
        }
        display.setTextColor(1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK) {
                    if (y_pos == 1) {
                        if (x_pos == 0) {
                            instrument->vol_loop = (loop_mode_t)(instrument->vol_loop + 1);
                            if (instrument->vol_loop > LOOP_CYLE) instrument->vol_loop = LOOP_SIGL;
                        } else if (x_pos == 1) {
                            instrument->vol_upend = !instrument->vol_upend;
                        } else if (x_pos == 2) {
                            instrument->vol_hold = !instrument->vol_hold;
                        } else if (x_pos == 3) {
                            instrument->vol_mode = (env_mode_t)!instrument->vol_mode;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_BACK) {
                    int8_t ret = boolWindowMenu("SAVE?", "WANT TO SAVE CHANGES?");
                    if (ret == 1) {
                        save_instrument(instrument->file_name, instrument);
                        break;
                    } else if (ret == 0) {
                        read_instrument(instrument->file_name, instrument);
                        break;
                    }
                } else if (optionKeyEvent.num == KEY_P) {
                    if (y_pos == 0) {
                        UpKeyStat = true;
                        instrument->vol_env[e_pos]++;
                        if (instrument->vol_env[e_pos] > 15) instrument->vol_env[e_pos] = 15;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->vol_rate++;
                            if (instrument->vol_rate > 64) instrument->vol_rate = 64;
                        } else if (spd_pos == 1) {
                            instrument->frq++;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_S) {
                    if (y_pos == 0) {
                        DownKeyStat = true;
                        instrument->vol_env[e_pos]--;
                        if (instrument->vol_env[e_pos] < 0) instrument->vol_env[e_pos] = 0;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->vol_rate--;
                            if (instrument->vol_rate < 1) instrument->vol_rate = 1;
                        } else if (spd_pos == 1) {
                            instrument->frq--;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_L) {
                    if (y_pos == 0) {
                        e_pos--;
                        if (e_pos < 0) e_pos = instrument->vol_len-1;
                    } else {
                        x_pos--;
                        if (x_pos < 0) x_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_R) {
                    if (y_pos == 0) {
                        e_pos++;
                        if (e_pos > instrument->vol_len-1) e_pos = 0;
                    } else {
                        x_pos++;
                        if (x_pos > 4) x_pos = 4;
                    }
                } else if (optionKeyEvent.num == KEY_UP) {
                    if (x_pos == 4) {
                        spd_pos = 0;
                    } else {
                        y_pos--;
                        if (y_pos < 0) y_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_pos == 4) {
                        spd_pos = 1;
                    } else {
                        y_pos++;
                        if (y_pos > 1) y_pos = 1;
                    }
                } else if (optionKeyEvent.num == KEY_NAVI) {
                    chipbox_config.enbLEDFadeIn = true;
                    enbOctvAdjust = false;
                    reset_all_note();
                    for (uint8_t i = 0; i < instrument->vol_len - e_fct; i++) {
                        led_stat[i] = true;
                    }
                    touchPadMode = true;
                    touchPadAutoLED = false;
                    fnKeyStat = true;
                } else if (optionKeyEvent.num == KEY_OCTU) {
                    if (!enbOctvAdjust) {
                        e_fct = 15;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->vol_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_OCTD) {
                    if (!enbOctvAdjust) {
                        e_fct = 0;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->vol_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_NAVI) {
                    for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                        led_stat[i] = false;
                    }
                    enbOctvAdjust = true;
                    reset_all_note();
                    fnKeyStat = false;
                    touchPadMode = false;
                    touchPadAutoLED = true;
                    chipbox_config.enbLEDFadeIn = false;
                } else if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                }
            }
        }
        if (readTouchPadKeyEvent == pdTRUE && touchPadEvent.status == KEY_ATTACK) {
            instrument->vol_len = touchPadEvent.num + e_fct + 1;
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_stat[i] = false;
            }
            for (uint8_t i = 0; i < instrument->vol_len - e_fct; i++) {
                led_stat[i] = true;
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->vol_env[e_pos]++;
                if (instrument->vol_env[e_pos] > 15) instrument->vol_env[e_pos] = 15;
            }
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->vol_env[e_pos]--;
                if (instrument->vol_env[e_pos] < 0) instrument->vol_env[e_pos] = 0;
            }
        }
        vTaskDelay(1);
    }
    display.setFont(&font3x5);
}

void lfo_env_edit(instruments_t *instrument) {
    key_event_t optionKeyEvent;
    key_event_t touchPadEvent;
    int8_t e_pos = 0;
    int8_t e_fct = 0;
    int8_t x_pos = 0;
    int8_t y_pos = 1;
    int8_t spd_pos = 0;
    bool UpKeyStat = false;
    uint8_t UpKeyTick = 0;
    bool DownKeyStat = false;
    uint8_t DownKeyTick = 0;
    uint8_t fctTick = 0;
    uint8_t cont;
    bool fnKeyStat = false;
    for (;;) {
        display.clearDisplay();
        display.drawBitmap(2, 1, VibratoTitle, 40, 8, 1);
        uint8_t len = instrument->lfo_len;
        display.drawFastHLine(0, 11, 128, 1);
        display.drawFastVLine(109, 0, 11, 1);
        display.drawFastHLine(0, 48, 128, 1);
        display.drawFastVLine(69, 49, 14, 1);
        if (len >= 1) {
            cont = 64;
        }
        if (len >= 2) {
            cont = 32;
        }
        if (len >= 4) {
            cont = 16;
        }
        if (len >= 8) {
            cont = 8;
        }
        if (len >= 16) {
            cont = 4;
        }
        if (len) {
            for (uint8_t index = 0; index < len; index++) {
                uint8_t h = instrument->lfo_env[index] * 2;
                bool now_pos = false;
                for (uint8_t n = 0; n < MAX_POLYPHONY; n++) {
                    if (notes[n].status == NOTE_ON && notes[n].lfo_tick == index) now_pos = true;
                }
                if (y_pos == 0 && e_pos == index) {
                    now_pos = true;
                    display.drawFastHLine((index * cont) + 1, 13, cont - 1, 1);
                }
                if (h) {
                    if (now_pos)
                        display.fillRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    else
                        display.drawRect((index * cont) + 1, 46 - h, cont-1, h, 1);
                    // printf("index=%d cont=%d re=%d rey=%d\n", index, cont, (index * cont) + 1, instrument->lfo_env[index] * 2);
                } else {
                    display.drawFastHLine((index * cont) + 1, 45, cont - 1, 1);
                }
            }
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            if (y_pos == 0) {
                display.printf("%3d", instrument->lfo_env[e_pos]);
            } else {
                display.printf(" ??");
            }
        } else {
            display.setFont(&font5x7);
            display.setCursor(111, 2);
            display.printf(" ??");
            display.setFont(&font3x5);
            display.setCursor(33, 27);
            display.printf("ENVELOPE IS NULL");
        }
        display.drawBitmap(4, 51, Loop_mode_icon[y_pos == 1 && x_pos == 0][instrument->lfo_loop], 11, 11, 1);
        if (instrument->lfo_upend) {
            display.drawBitmap(20, 51, Reverse[y_pos == 1 && x_pos == 1], 11, 11, 1);
        } else {
            display.drawBitmap(20, 51, PositiveDirection[y_pos == 1 && x_pos == 1], 11, 11, 1);
        }
        if (instrument->lfo_hold) {
            display.drawBitmap(36, 51, Hold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        } else {
            display.drawBitmap(36, 51, UnHold[y_pos == 1 && x_pos == 2], 11, 11, 1);
        }
        if (instrument->lfo_mode == ENV_DISC) {
            display.drawBitmap(52, 51, Dispersed[y_pos == 1 && x_pos == 3], 11, 11, 1);
        } else if (instrument->lfo_mode == ENV_CONT) {
            display.drawBitmap(52, 51, Continuity[y_pos == 1 && x_pos == 3], 11, 11, 1);
        }
        display.setFont(&font3x5);
        display.setCursor(92, 51);
        display.printf("RATE");
        if (instrument->tick_mode == TICK_FRQ) {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->lfo_rate);
        } else {
            if (x_pos == 4 && spd_pos == 0) {
                display.fillRect(108, 50, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[1/%d]", instrument->lfo_rate);
        }
        display.setTextColor(1);
        display.setCursor(92, 58);
        if (instrument->tick_mode == TICK_FRQ) {
            display.printf("FREQ");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->frq);
        } else {
            display.printf(" BPM");
            if (x_pos == 4 && spd_pos == 1) {
                display.fillRect(108, 57, 19, 7, 1);
                display.setTextColor(0);
            }
            display.printf("[%3d]", instrument->bpm);
        }
        display.setTextColor(1);
        display.display();
        if (readOptionKeyEvent == pdTRUE) {
            if (optionKeyEvent.status == KEY_ATTACK) {
                if (optionKeyEvent.num == KEY_OK) {
                    if (y_pos == 1) {
                        if (x_pos == 0) {
                            instrument->lfo_loop = (loop_mode_t)(instrument->lfo_loop + 1);
                            if (instrument->lfo_loop > LOOP_CYLE) instrument->lfo_loop = LOOP_SIGL;
                        } else if (x_pos == 1) {
                            instrument->lfo_upend = !instrument->lfo_upend;
                        } else if (x_pos == 2) {
                            instrument->lfo_hold = !instrument->lfo_hold;
                        } else if (x_pos == 3) {
                            instrument->lfo_mode = (env_mode_t)!instrument->lfo_mode;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_BACK) {
                    int8_t ret = boolWindowMenu("SAVE?", "WANT TO SAVE CHANGES?");
                    if (ret == 1) {
                        save_instrument(instrument->file_name, instrument);
                        break;
                    } else if (ret == 0) {
                        read_instrument(instrument->file_name, instrument);
                        break;
                    }
                } else if (optionKeyEvent.num == KEY_P) {
                    if (y_pos == 0) {
                        UpKeyStat = true;
                        instrument->lfo_env[e_pos]++;
                        if (instrument->lfo_env[e_pos] > 15) instrument->lfo_env[e_pos] = 15;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->lfo_rate++;
                            if (instrument->lfo_rate > 64) instrument->lfo_rate = 64;
                        } else if (spd_pos == 1) {
                            instrument->frq++;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_S) {
                    if (y_pos == 0) {
                        DownKeyStat = true;
                        instrument->lfo_env[e_pos]--;
                        if (instrument->lfo_env[e_pos] < 0) instrument->lfo_env[e_pos] = 0;
                    } else if (x_pos == 4) {
                        if (spd_pos == 0) {
                            instrument->lfo_rate--;
                            if (instrument->lfo_rate < 1) instrument->lfo_rate = 1;
                        } else if (spd_pos == 1) {
                            instrument->frq--;
                        }
                        refresh_crossover();
                    }
                } else if (optionKeyEvent.num == KEY_L) {
                    if (y_pos == 0) {
                        e_pos--;
                        if (e_pos < 0) e_pos = instrument->lfo_len-1;
                    } else {
                        x_pos--;
                        if (x_pos < 0) x_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_R) {
                    if (y_pos == 0) {
                        e_pos++;
                        if (e_pos > instrument->lfo_len-1) e_pos = 0;
                    } else {
                        x_pos++;
                        if (x_pos > 4) x_pos = 4;
                    }
                } else if (optionKeyEvent.num == KEY_UP) {
                    if (x_pos == 4) {
                        spd_pos = 0;
                    } else {
                        y_pos--;
                        if (y_pos < 0) y_pos = 0;
                    }
                } else if (optionKeyEvent.num == KEY_DOWN) {
                    if (x_pos == 4) {
                        spd_pos = 1;
                    } else {
                        y_pos++;
                        if (y_pos > 1) y_pos = 1;
                    }
                } else if (optionKeyEvent.num == KEY_NAVI) {
                    chipbox_config.enbLEDFadeIn = true;
                    enbOctvAdjust = false;
                    reset_all_note();
                    for (uint8_t i = 0; i < instrument->lfo_len - e_fct; i++) {
                        led_stat[i] = true;
                    }
                    touchPadMode = true;
                    touchPadAutoLED = false;
                    fnKeyStat = true;
                } else if (optionKeyEvent.num == KEY_OCTU) {
                    if (!enbOctvAdjust) {
                        e_fct = 15;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->lfo_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                } else if (optionKeyEvent.num == KEY_OCTD) {
                    if (!enbOctvAdjust) {
                        e_fct = 0;
                        for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                            led_stat[i] = false;
                        }
                        for (uint8_t i = 0; i < instrument->lfo_len - e_fct; i++) {
                            led_stat[i] = true;
                        }
                    }
                }
            } else if (optionKeyEvent.status == KEY_RELEASE) {
                if (optionKeyEvent.num == KEY_NAVI) {
                    for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                        led_stat[i] = false;
                    }
                    enbOctvAdjust = true;
                    reset_all_note();
                    fnKeyStat = false;
                    touchPadMode = false;
                    touchPadAutoLED = true;
                    chipbox_config.enbLEDFadeIn = false;
                } else if (optionKeyEvent.num == KEY_P) {
                    UpKeyStat = false;
                    UpKeyTick = 0;
                } else if (optionKeyEvent.num == KEY_S) {
                    DownKeyStat = false;
                    DownKeyTick = 0;
                }
            }
        }
        if (readTouchPadKeyEvent == pdTRUE && touchPadEvent.status == KEY_ATTACK) {
            instrument->lfo_len = touchPadEvent.num + e_fct + 1;
            for (uint8_t i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
                led_stat[i] = false;
            }
            for (uint8_t i = 0; i < instrument->lfo_len - e_fct; i++) {
                led_stat[i] = true;
            }
        }
        if (UpKeyTick < 128) {
            if (UpKeyStat)
                UpKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->lfo_env[e_pos]++;
                if (instrument->lfo_env[e_pos] > 15) instrument->lfo_env[e_pos] = 15;
            }
        }
        if (DownKeyTick < 128) {
            if (DownKeyStat)
                DownKeyTick++;
        } else {
            fctTick++;
            if (fctTick > 7) {
                fctTick = 0;
                instrument->lfo_env[e_pos]--;
                if (instrument->lfo_env[e_pos] < 0) instrument->lfo_env[e_pos] = 0;
            }
        }
        vTaskDelay(1);
    }
    display.setFont(&font3x5);
}