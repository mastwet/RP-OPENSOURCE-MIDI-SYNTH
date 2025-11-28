#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "src_config.h"

extern TaskHandle_t SOUND_ENG;
#define VERSION 6

typedef enum {
    WAVE_P125,
    WAVE_P025,
    WAVE_P050,
    WAVE_P075,
    WAVE_TRIG,
    WAVE_SINE,
    WAVE_SAWT,
    WAVE_NOSE,
    WAVE_USER
} wave_type_t;

typedef enum {
    ENV_DISC, // 离散模式
    ENV_CONT  // 连续模式
} env_mode_t;

typedef enum {
    LOOP_SIGL, // 单次循环
    LOOP_STEP, // 步进循环
    LOOP_CYLE  // 周期循环
} loop_mode_t;

typedef enum {
    TICK_FRQ,  // 频率模式
    TICK_BPM   // BPM模式
} tick_mode_t;

typedef struct {
    char CI_char[4] = "CPI"; // 文件头
    uint8_t version = CHIPBOX_VERSION; // 乐器版本
    char name[13] = "NEWINST"; // 乐器名称，最多12个字符，最后一个字符为null终止符
    uint8_t profile_len = 11; // 简介长度
    bool enb_vol_env = false;
    bool enb_arp_env = false;
    bool enb_lfo_env = false;
    bool enb_dty_env = false;
    uint8_t vol_len = 0; // 音量包络长度
    uint8_t arp_len = 0; // 琶音包络长度
    uint8_t lfo_len = 0; // LFO包络长度
    uint8_t dty_len = 0; // 占空比包络长度
    env_mode_t vol_mode = ENV_DISC; // 音量包络模式
    env_mode_t arp_mode = ENV_DISC; // 琶音包络模式
    env_mode_t lfo_mode = ENV_DISC; // LFO包络模式
    uint8_t vol_factor = 0; // 音量连续模式插值倍率
    uint8_t arp_factor = 0; // 琶音连续模式插值倍率
    uint8_t lfo_factor = 0; // LFO连续模式插值倍率
    bool vol_hold = 0; // HOLD
    bool arp_hold = 0; // HOLD
    bool lfo_hold = 0; // HOLD
    bool dty_hold = 0; // HOLD
    bool vol_upend = 0; // 是否倒放
    bool arp_upend = 0; // 是否倒放
    bool lfo_upend = 0; // 是否倒放
    bool dty_upend = 0; // 是否倒放
    loop_mode_t vol_loop = LOOP_SIGL; // 音量包络循环模式
    loop_mode_t arp_loop = LOOP_SIGL; // 琶音包络循环模式
    loop_mode_t lfo_loop = LOOP_SIGL; // LFO包络循环模式
    loop_mode_t dty_loop = LOOP_SIGL; // 占空比包络循环模式
    wave_type_t wave = WAVE_P125; // 波表类型
    tick_mode_t tick_mode = TICK_FRQ; // tick模式
    uint8_t bpm = 125; // BPM模式下有效
    uint8_t frq = 60; // FRQ模式下有效
    // rate（在BPM下，正数表示1/x bar，负数表示x bar。在FRQ模式下，表示分频系数，不可为负数）
    int8_t vol_rate = 1; // 音量包络rate
    int8_t arp_rate = 1; // 琶音包络rate
    int8_t lfo_rate = 1; // LFO包络rate
    int8_t dty_rate = 1; // 占空比包络rate
    char profile[64] = "PROFILE..."; // 简介
    int8_t vol_env[32]; // 音量包络数据指针（音量范围是0-15）
    int8_t arp_env[32]; // 琶音包络数据指针
    int8_t lfo_env[32]; // LFO包络数据指针
    int8_t dty_env[32]; // 占空比包络数据指针
    int8_t arp_map_num = 0; // 音阶
    int8_t arp_root_key = 0; // 根音
    char file_name[32]; // 文件名，ChipBOX内部使用，不读写
} instruments_t;

void save_instrument(const char *filename, instruments_t *instrument) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Write Instrument Error\n");
        return;
    }

    if (instrument == NULL) {
        printf("Create New Instrument..\n");
        instruments_t new_inst;
        memset(new_inst.vol_env, 0, sizeof(new_inst.vol_env));
        memset(new_inst.arp_env, 0, sizeof(new_inst.arp_env));
        memset(new_inst.lfo_env, 0, sizeof(new_inst.lfo_env));
        memset(new_inst.dty_env, 0, sizeof(new_inst.dty_env));
        fwrite(&new_inst, sizeof(instruments_t), 1, file);
    } else {
        printf("Write Instrument..\n");
        strcpy(instrument->CI_char, "CPI");
        fwrite(instrument, sizeof(instruments_t), 1, file);
    }

    fclose(file);
}

int8_t read_instrument(const char *filename, instruments_t *instrument) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Read Instrument Error\n");
        return -1;
    }

    fread(instrument, sizeof(instruments_t), 1, file);
    strncpy(instrument->file_name, filename, 31);
    printf("READ FILE: %s\nCPY: %s\n", filename, instrument->file_name);

    if (strcmp(instrument->CI_char, "CPI")) {
        printf("This is NOT a ChipBOX Instrument. HEAD=%s\n", instrument->CI_char);
        fclose(file);
        return -2;
    }

    fclose(file);
    return 0;
}

void print_current_env(int8_t *env, uint8_t len) {
    for (int i = 0; i < len; i++) {
        printf("%d ", env[i]);
    }
    printf("\n");
}

void view_print_env(int8_t *env, uint8_t len) {
    const int height = 16; // Assuming the envelope value range is 0-15
    char graph[height][len];
    memset(graph, ' ', sizeof(graph));

    for (int i = 0; i < len; i++) {
        if (env[i] >= 0 && env[i] < height) {
            graph[height - env[i] - 1][i] = '#';
        }
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < len; j++) {
            printf("%c", graph[i][j]);
        }
        printf("\n");
    }
}

void print_instrument(instruments_t *instrument) {
    printf("Instrument Name: %s\n", instrument->name);
    printf("Instrument Version: %d (V%.1f)\n", instrument->version, instrument->version / 10.0f);
    if (instrument->profile_len && instrument->profile != NULL) {
        printf("Instrument Profile:\n%s\nProfile Size: %dchars\n", instrument->profile, instrument->profile_len);
    } else {
        printf("Instrument Profile: NULL\n");
    }

    printf("Volume Envelope Length: %d\n", instrument->vol_len);
    printf("Arpeggio Envelope Length: %d\n", instrument->arp_len);
    printf("LFO Envelope Length: %d\n", instrument->lfo_len);
    printf("Duty Envelope Length: %d\n", instrument->dty_len);

    printf("\n");
    printf("Volume Envelope: ");
    print_current_env(instrument->vol_env, instrument->vol_len);
    printf("Arpeggio Envelope: ");
    print_current_env(instrument->arp_env, instrument->arp_len);
    printf("LFO Envelope: ");
    print_current_env(instrument->lfo_env, instrument->lfo_len);
    printf("Duty Envelope: ");
    print_current_env(instrument->dty_env, instrument->dty_len);

    printf("\n");
    printf("Volume Mode: %s\n", instrument->vol_mode == ENV_DISC ? "Discrete" : "Continuous");
    printf("Arpeggio Mode: %s\n", instrument->arp_mode == ENV_DISC ? "Discrete" : "Continuous");
    printf("LFO Mode: %s\n", instrument->lfo_mode == ENV_DISC ? "Discrete" : "Continuous");

    printf("Volume Continuous Factor: %dx\n", instrument->vol_factor);
    printf("Arpeggio Continuous Factor: %dx\n", instrument->arp_factor);
    printf("LFO Continuous Factor: %dx\n", instrument->lfo_factor);

    printf("\n");
    printf("Volume Loop: %s\n", instrument->vol_loop == LOOP_SIGL ? "Single" : instrument->vol_loop == LOOP_STEP ? "Step" : "Cycle");
    printf("Arpeggio Loop: %s\n", instrument->arp_loop == LOOP_SIGL ? "Single" : instrument->arp_loop == LOOP_STEP ? "Step" : "Cycle");
    printf("LFO Loop: %s\n", instrument->lfo_loop == LOOP_SIGL ? "Single" : instrument->lfo_loop == LOOP_STEP ? "Step" : "Cycle");
    printf("Duty Loop: %s\n", instrument->dty_loop == LOOP_SIGL ? "Single" : instrument->dty_loop == LOOP_STEP ? "Step" : "Cycle");

    printf("\n");
    const char *wave_types[] = {"WAVE_P125", "WAVE_P025", "WAVE_P050", "WAVE_P075", "WAVE_TRIG", "WAVE_SINE", "WAVE_SAWT", "WAVE_NOSE", "WAVE_USER"};
    // printf("Wave Type: %s\n", wave_types[instrument->wave]);
    printf("Wave Type: %d\n", instrument->wave);

    printf("\n");
    printf("TICK_MODE: %s\n", instrument->tick_mode == TICK_FRQ ? "TICK_FRQ" : "TICK_BPM");
    printf("BPM: %d (Valid for TICK_MODE == TICK_BPM)\n", instrument->bpm);
    printf("FRQ: %dHz (Valid for TICK_MODE == TICK_FRQ)\n", instrument->frq);

    if (instrument->tick_mode == TICK_FRQ) {
        if (instrument->vol_rate < 1) printf("Volume Rate: invalid(%d)\n", instrument->vol_rate);
        else printf("Volume Rate: %dTick\n", instrument->vol_rate);

        if (instrument->arp_rate < 1) printf("Arpeggio Rate: invalid(%d)\n", instrument->arp_rate);
        else printf("Arpeggio Rate: %dTick\n", instrument->arp_rate);

        if (instrument->lfo_rate < 1) printf("LFO Rate: invalid(%d)\n", instrument->lfo_rate);
        else printf("LFO Rate: %dTick\n", instrument->lfo_rate);

        if (instrument->dty_rate < 1) printf("Duty Rate: invalid(%d)\n", instrument->dty_rate);
        else printf("Duty Rate: %dTick\n", instrument->dty_rate);
    } else {
        if (instrument->vol_rate < 0) printf("Volume Rate: %dbar\n", -instrument->vol_rate);
        else if (instrument->vol_rate > 0) printf("Volume Rate: 1/%dbar\n", instrument->vol_rate);
        else printf("Volume Rate: invalid(%d)\n", instrument->vol_rate);

        if (instrument->arp_rate < 0) printf("Arpeggio Rate: %dbar\n", -instrument->arp_rate);
        else if (instrument->arp_rate > 0) printf("Arpeggio Rate: 1/%dbar\n", instrument->arp_rate);
        else printf("Arpeggio Rate: invalid(%d)\n", instrument->arp_rate);

        if (instrument->lfo_rate < 0) printf("LFO Rate: %dbar\n", -instrument->lfo_rate);
        else if (instrument->lfo_rate > 0) printf("LFO Rate: 1/%dbar\n", instrument->lfo_rate);
        else printf("LFO Rate: invalid(%d)\n", instrument->lfo_rate);

        if (instrument->dty_rate < 0) printf("Duty Rate: %dbar\n", -instrument->dty_rate);
        else if (instrument->dty_rate > 0) printf("Duty Rate: 1/%dbar\n", instrument->dty_rate);
        else printf("Duty Rate: invalid(%d)\n", instrument->dty_rate);
    }
}

void set_current_env_length(instruments_t *instrument, uint8_t len, int8_t env[32]) {
    for (uint8_t i = instrument->vol_len; i < len; i++) {
        env[i] = 0;
    }
}

void set_current_env_value(int8_t *env, uint8_t index, int8_t value, uint8_t len) {
    if (index < len) {
        env[index] = value;
    } else {
        printf("Index out of bounds\n");
    }
}

void set_current_env_values(int8_t *env, uint8_t start, int8_t *values, uint8_t len, uint8_t values_len) {
    if (start + values_len <= len) {
        for (uint8_t i = 0; i < values_len; i++) {
            env[start + i] = values[i];
        }
    } else {
        printf("Values exceed envelope length\n");
    }
}

void print_help() {
    printf("Available commands:\n");
    printf("  load <file_path> - Load an instrument from a file\n");
    printf("  save - Save the current instrument to the currently loaded file\n");
    printf("  save_as <file_path> - Save the current instrument to a new file\n");
    printf("  current_file - Print the current instrument file and name\n");
    printf("  current_env - Print current envelope option\n");
    printf("  goto <vol/arp/lfo/dty> - Select the envelope to edit\n");
    printf("  print - Print the current envelope\n");
    printf("  view_print - Visualize the current envelope\n");
    printf("  rename <name> - Rename the instrument\n");
    printf("  edit_profile - Start editing the profile of this instrument\n");
    printf("  print_all - Print all information about the current instrument\n");
    printf("  set_loop_mode <LOOP_SIGL/LOOP_STEP/LOOP_CYLE> - Set the loop mode for the current envelope\n");
    printf("  set_env_mode <ENV_DISC/ENV_CONT> - Set the envelope mode for the current envelope\n");
    printf("  set_wave <WAVE_P125/WAVE_P025/WAVE_P050/WAVE_P075/WAVE_TRIG/WAVE_SINE/WAVE_SAWT/WAVE_NOSE/WAVE_USER> - Set the wave type for the instrument\n");
    printf("  set_rate <rate> - Set the Rate for the current envelope\n");
    printf("  set_factor <factor> - Set Continuous Factor for the current envelope\n");
    printf("  set_tick_mode <TICK_FRQ/TICK_BPM> - Set BPM (Valid for TICK_MODE == TICK_BPM)\n");
    printf("  set_bpm <bpm> - Set BPM (Valid for TICK_MODE == TICK_BPM)\n");
    printf("  set_frq <frq> - Set FRQ (Valid for TICK_MODE == TICK_FRQ)\n");
    printf("  set <index> <value> - Set the value of the current envelope at the specified index\n");
    printf("  set_cont <start> <values> - Set consecutive values in the current envelope starting from the specified index\n");
    printf("  set_len <length> - Set the length of the current envelope\n");
    printf("  version - Get version\n");
    printf("  exit - Exit the editor\n");
    printf("(RATE in BPM mode, a positive number indicates 1/x bar, a negative number indicates x bar.\nin FRQ mode, it indicates the crossover coefficient, which cannot be negative)\n");
    printf("  qwq\n");
}

void fgets_r(char *buffer, int length) {
    int index = 0;
    while (1) {
        if (Serial.available() > 0) {
            char incomingByte = Serial.read();
            if (incomingByte == 127 || incomingByte == 8) {
                if (index > 0) {
                    index--;
                    Serial.print("\b \b");
                }
            } else if (incomingByte == '\n' || incomingByte == '\r') {
                buffer[index] = '\0';
                Serial.println();
                break;
            } else {
                if (index < length - 1) {
                    buffer[index++] = incomingByte;
                    Serial.print(incomingByte);
                }
            }
        }
        vTaskDelay(1);
    }
}

void profile_editor(instruments_t *instrument) {
    char buffer[16];  // 每行最多输入15个字符，加上一个'\0'终止符
    char temp_profile[64] = "";  // 临时存储所有输入的缓冲区
    int total_chars = 0, lines = 0;

    printf("Enter profile (max 4 lines, 15 chars per line):\n");

    while (lines < 4) {
        printf("Line %d> ", lines + 1);
        fflush(stdout);
        fgets_r(buffer, 16);

        int len = strlen(buffer);

        if (len == 0) {
            break;
        }

        if (total_chars + len + (lines > 0 ? 1 : 0) > 64) {
            printf("Error: Profile exceeds 60 characters limit.\n");
            break;
        }

        if (lines > 0) {
            strcat(temp_profile, "\n");
        }
        strcat(temp_profile, buffer);
        total_chars += len + 1;
        // printf("Now total_chars %d\n", total_chars);
        lines++;
    }

    strcpy(instrument->profile, temp_profile);  // 复制临时简介到最终位置
    instrument->profile_len = total_chars;

    printf("Instrument Profile:\n%s\n\nNumber of characters: %d\n", instrument->profile, instrument->profile_len);
}

void interactive_menu(instruments_t *instrument, char *filename) {
    char command[512];
    char item[10];
    char file_path[128];
    int8_t value;
    uint8_t index, len;
    uint8_t env_type = 0;
    int8_t values[256];
    int values_len;

    while (1) {
        if (strlen(instrument->name)) printf("INSTRUMENT %s [", instrument->name);
        else printf("INSTRUMENT EDITOR [");
        switch (env_type) {
            case 0: printf("vol"); break;
            case 1: printf("arp"); break;
            case 2: printf("lfo"); break;
            case 3: printf("dty"); break;
        }
        printf("]>>> ");
        fflush(stdout);
        fgets_r(command, sizeof(command));

        if (strncmp(command, "load ", 5) == 0) {
            sscanf(command + 5, "%s", file_path);
            read_instrument(file_path, instrument);
            strncpy(filename, file_path, 100);
            printf("Instrument loaded from %s\n", file_path);
        } else if (strcmp(command, "save") == 0) {
            if (strlen(filename) == 0) {
                printf("No file is currently loaded. Use 'save_as' to specify a file.\n");
            } else {
                save_instrument(filename, instrument);
                printf("Instrument saved to %s\n", filename);
            }
        } else if (strncmp(command, "save_as ", 8) == 0) {
            sscanf(command + 8, "%s", file_path);
            save_instrument(file_path, instrument);
            strncpy(filename, file_path, 100);
            printf("Instrument saved to %s\n", file_path);
        } else if (strcmp(command, "current_file") == 0) {
            printf("Current file: %s\n", filename);
            printf("Instrument name: %s\n", instrument->name);
        } else if (strncmp(command, "goto ", 5) == 0) {
            sscanf(command + 5, "%s", item);
            if (strcmp(item, "vol") == 0) env_type = 0;
            else if (strcmp(item, "arp") == 0) env_type = 1;
            else if (strcmp(item, "lfo") == 0) env_type = 2;
            else if (strcmp(item, "dty") == 0) env_type = 3;
            else printf("Invalid envelope type. Use vol, arp, lfo, or dty.\n");
        } else if (strcmp(command, "print") == 0) {
            switch (env_type) {
                case 0: print_current_env(instrument->vol_env, instrument->vol_len); break;
                case 1: print_current_env(instrument->arp_env, instrument->arp_len); break;
                case 2: print_current_env(instrument->lfo_env, instrument->lfo_len); break;
                case 3: print_current_env(instrument->dty_env, instrument->dty_len); break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
        } else if (strcmp(command, "view_print") == 0) {
            switch (env_type) {
                case 0: view_print_env(instrument->vol_env, instrument->vol_len); break;
                case 1: view_print_env(instrument->arp_env, instrument->arp_len); break;
                case 2: view_print_env(instrument->lfo_env, instrument->lfo_len); break;
                case 3: view_print_env(instrument->dty_env, instrument->dty_len); break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
        } else if (strncmp(command, "rename ", 7) == 0) {
            char newName[32];
            // 从 command+7 的位置读取直到行尾，包括空格
            if (sscanf(command + 7, "%31[^\n]", newName) == 1) { // 使用31来限制最大字符数，为 '\0' 留出空间
                if (strlen(newName) > 12) {
                    printf("Error: Name exceeds 12 characters.\n");
                } else {
                    strcpy(instrument->name, newName);
                    printf("Instrument renamed to %s\n", instrument->name);
                }
            } else {
                printf("Error: Failed to read new name.\n");
            }
        } else if (strcmp(command, "edit_profile") == 0) {
            profile_editor(instrument);
        } else if (strcmp(command, "print_all") == 0) {
            print_instrument(instrument);
        } else if (strncmp(command, "set_loop_mode ", 14) == 0) {
            sscanf(command + 14, "%s", item);
            loop_mode_t loop_mode;
            if (strcmp(item, "LOOP_SIGL") == 0) loop_mode = LOOP_SIGL;
            else if (strcmp(item, "LOOP_STEP") == 0) loop_mode = LOOP_STEP;
            else if (strcmp(item, "LOOP_CYLE") == 0) loop_mode = LOOP_CYLE;
            else {
                printf("Invalid loop mode. Use LOOP_SIGL, LOOP_STEP, or LOOP_CYLE.\n");
                continue;
            }
            switch (env_type) {
                case 0: instrument->vol_loop = loop_mode; break;
                case 1: instrument->arp_loop = loop_mode; break;
                case 2: instrument->lfo_loop = loop_mode; break;
                case 3: instrument->dty_loop = loop_mode; break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
            printf("Loop mode set to %s\n", item);
        } else if (strncmp(command, "set_tick_mode ", 14) == 0) {
            sscanf(command + 14, "%s", item);
            if (strcmp(item, "TICK_FRQ") == 0) instrument->tick_mode = TICK_FRQ;
            else if (strcmp(item, "TICK_BPM") == 0) instrument->tick_mode = TICK_BPM;
            else {
                printf("Invalid tick mode. Use TICK_FRQ or TICK_BPM.\n");
                continue;
            }
            printf("Tick mode set to %s\n", item);
        } else if (strncmp(command, "set_rate ", 9) == 0) {
            int8_t rate;
            sscanf(command + 9, "%d", &rate);
            switch (env_type) {
                case 0: instrument->vol_rate = rate; break;
                case 1: instrument->arp_rate = rate; break;
                case 2: instrument->lfo_rate = rate; break;
                case 3: instrument->dty_rate = rate; break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
            if (instrument->tick_mode == TICK_FRQ) {
                if (rate > 0) printf("Rate set to %d\n", rate);
                else printf("Rate NOT VALID\n");
            } else {
                if (rate < 0) printf("Rate set to %dbar\n", -rate);
                else if (rate > 0) printf("Rate set to 1/%dbar\n", rate);
                else printf("Rate NOT VALID\n");
            }
        } else if (strncmp(command, "set_factor ", 11) == 0) {
            uint8_t factor;
            sscanf(command + 11, "%d", &factor);
            switch (env_type) {
                case 0: instrument->vol_factor = factor; break;
                case 1: instrument->arp_factor = factor; break;
                case 2: instrument->lfo_factor = factor; break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
            printf("Set Continuous Factor to %dx\n", factor);
        } else if (strncmp(command, "set_frq ", 8) == 0) {
            sscanf(command + 8, "%d", &instrument->frq);
            printf("Freq set to %dHz\n", instrument->frq);
        } else if (strncmp(command, "set_bpm ", 8) == 0) {
            sscanf(command + 8, "%d", &instrument->bpm);
            printf("BPM set to %d\n", instrument->bpm);
        } else if (strncmp(command, "set_env_mode ", 13) == 0) {
            sscanf(command + 13, "%s", item);
            env_mode_t env_mode;
            if (strcmp(item, "ENV_DISC") == 0) env_mode = ENV_DISC;
            else if (strcmp(item, "ENV_CONT") == 0) env_mode = ENV_CONT;
            else {
                printf("Invalid envelope mode. Use ENV_DISC or ENV_CONT.\n");
                continue;
            }
            switch (env_type) {
                case 0: instrument->vol_mode = env_mode; break;
                case 1: instrument->arp_mode = env_mode; break;
                case 2: instrument->lfo_mode = env_mode; break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
            printf("Envelope mode set to %s\n", item);
        } else if (strncmp(command, "set_wave ", 9) == 0) {
            sscanf(command + 9, "%s", item);
            if (strcmp(item, "WAVE_P125") == 0) instrument->wave = WAVE_P125;
            else if (strcmp(item, "WAVE_P025") == 0) instrument->wave = WAVE_P025;
            else if (strcmp(item, "WAVE_P050") == 0) instrument->wave = WAVE_P050;
            else if (strcmp(item, "WAVE_P075") == 0) instrument->wave = WAVE_P075;
            else if (strcmp(item, "WAVE_TRIG") == 0) instrument->wave = WAVE_TRIG;
            else if (strcmp(item, "WAVE_SINE") == 0) instrument->wave = WAVE_SINE;
            else if (strcmp(item, "WAVE_SAWT") == 0) instrument->wave = WAVE_SAWT;
            else if (strcmp(item, "WAVE_NOSE") == 0) instrument->wave = WAVE_NOSE;
            else if (strcmp(item, "WAVE_USER") == 0) instrument->wave = WAVE_USER;
            else {
                printf("Invalid wave type. Use WAVE_P125, WAVE_P025, WAVE_P050, WAVE_P075, WAVE_TRIG, WAVE_SINE, WAVE_SAWT, WAVE_NOSE, or WAVE_USER.\n");
                continue;
            }
            printf("Wave type set to %s\n", item);
        } else if (strncmp(command, "set ", 4) == 0) {
            if (sscanf(command + 4, "%d %d", &index, &value) == 2) {
                switch (env_type) {
                    case 0: set_current_env_value(instrument->vol_env, index, value, instrument->vol_len); break;
                    case 1: set_current_env_value(instrument->arp_env, index, value, instrument->arp_len); break;
                    case 2: set_current_env_value(instrument->lfo_env, index, value, instrument->lfo_len); break;
                    case 3: set_current_env_value(instrument->dty_env, index, value, instrument->dty_len); break;
                    default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
                }
                printf("Set value at index %d to %d\n", index, value);
            } else {
                printf("Usage: set <index> <value>\n");
            }
        } else if (strncmp(command, "set_cont ", 9) == 0) {
            char *token = strtok(command + 9, " ");
            index = atoi(token);
            values_len = 0;
            while (token != NULL && values_len < 256) {
                token = strtok(NULL, " ");
                if (token) {
                    values[values_len++] = atoi(token);
                }
            }
            switch (env_type) {
                case 0: set_current_env_values(instrument->vol_env, index, values, instrument->vol_len, values_len); break;
                case 1: set_current_env_values(instrument->arp_env, index, values, instrument->arp_len, values_len); break;
                case 2: set_current_env_values(instrument->lfo_env, index, values, instrument->lfo_len, values_len); break;
                case 3: set_current_env_values(instrument->dty_env, index, values, instrument->dty_len, values_len); break;
                default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
            }
            printf("Set consecutive values starting from index %d\n", index);
        } else if (strncmp(command, "set_len ", 8) == 0) {
            if (sscanf(command + 8, "%d", &len) == 1) {
                switch (env_type) {
                    case 0: instrument->vol_len = len; set_current_env_length(instrument, len, instrument->vol_env); break;
                    case 1: instrument->arp_len = len; set_current_env_length(instrument, len, instrument->arp_env); break;
                    case 2: instrument->lfo_len = len; set_current_env_length(instrument, len, instrument->lfo_env); break;
                    case 3: instrument->dty_len = len; set_current_env_length(instrument, len, instrument->dty_env); break;
                    default: printf("Invalid envelope type. Use goto to select an envelope.\n"); break;
                }
                if (len == 0) {
                    printf("This function has been disabled\n");
                } else {
                    printf("Set length of current envelope to %d\n", len);
                }
            } else {
                printf("Usage: set_len <length>\n");
            }
        } else if (strcmp(command, "current_env") == 0) {
            switch (env_type) {
                case 0: printf("vol\n"); break;
                case 1: printf("arp\n"); break;
                case 2: printf("lfo\n"); break;
                case 3: printf("dty\n"); break;
            }
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "version") == 0) {
            printf("ChipBOX Instrument Editor V%.1f (Std Edition)\nby libchara-dev\nBuild Date: %s %s\n", VERSION / 10.0f, __DATE__, __TIME__);
        } else {
            printf("Unknown command. Type 'help' for a list of commands.\n");
        }
    }
}
#endif