#ifndef NOTE_MAP_TYPE_H
#define NOTE_MAP_TYPE_H

#include <stdio.h>
#include <stdint.h>

typedef struct {
    char name[6]; // 简称，5个字符+一个终止符\0
    char full_name[16]; // 全称，15个个字符+一个终止符\0
    char profile[36]; // 简介，36字符，包含换行符和终止符
    uint8_t note_map_len = 0; // note map表长度，默认为0，不得大于12
    uint8_t note_map[12]; // 实际的映射数据
} scale_mode_data_t;

#define SCALE_MAX_ITEM 20
/*
scale_mode_data_t scale_mode_data_expm[SCALE_MAX_ITEM] = {
 // {"NAME ", "FULNAME", "   PROFILE   ",LEN,{   NOTE MAP DATA    }},
    {"C_MAJ", "C MAJOR", "MAJOR SCALE\n", 7, {0, 2, 4, 5, 7, 9, 11}}, // NO 1
    {"D_MIN", "D MINOR", "MINOR SCALE\n", 7, {2, 3, 5, 7, 9, 10, 12}} // NO 2
    // ...                                                            // NO ...
};
size_t scale_mode_data_t_size = sizeof(scale_mode_data_t);
*/

const scale_mode_data_t scale_data[SCALE_MAX_ITEM] = {
    {"CHROM","CHROMATIC SCALE","ALL SEMITONES",12,{0,1,2,3,4,5,6,7,8,9,10,11}},
    {"MAJOR","MAJOR SCALE","HAPPY AND GOOD FEEL",7,{0,2,4,5,7,9,11}},
    {"MINOR","NATURAL MINOR","SAD AND MELANCHOLIC",7,{0,2,3,5,7,8,10}},
    {"M_MIN","MELODIC MINOR","ASCENDING AND DESCENDING",7,{0,2,3,5,7,9,11}},
    {"H_MIN","HARMONIC MINOR","MIDDLE EASTERN FEEL",7,{0,2,3,5,7,8,11}},
    {"CHINA","CHINESE SCALE","TRADITIONAL CHINESE SOUND",5,{0,2,4,7,9}},
    {"JAPAN","JAPANESE SCALE","TRADITIONAL JAPANESE SOUND",5,{0,1,5,7,10}},
    {"BLUES","BLUES SCALE","BLUESY AND SOULFUL",6,{0,3,5,6,7,10}},
    {"DORIA","DORIAN SCALE","JAZZY AND FUNKY",7,{0,2,3,5,7,9,10}},
    {"PHRYG","PHRYGIAN SCALE","SPANISH AND FLAMENCO",7,{0,1,3,5,7,8,10}},
    {"LYDIA","LYDIAN SCALE","DREAMY AND LUSH",7,{0,2,4,6,7,9,11}},
    {"MIXOL","MIXOLYDIAN","BLUESY AND ROCK",7,{0,2,4,5,7,9,10}},
    {"LOCRI","LOCRIAN SCALE","DARK AND DISSONANT",7,{0,1,3,5,6,8,10}},
    {"WHOLE","WHOLE TONE","MYSTERIOUS AND AMBIGUOUS",6,{0,2,4,6,8,10}},
    {"H-W_D","HALF-WHOLE DIM","HALF-WHOLE DIMINISHED",8,{0,1,3,4,6,7,9,10}},
    {"W-H_D","WHOLE-HALF DIM","WHOLE-HALF DIMINISHED",8,{0,2,3,5,6,8,9,11}},
    {"BEBOM","BEBOP MAJOR","JAZZ IMPROVISATION",8,{0,2,4,5,7,8,9,11}},
    {"BEBOD","BEBOP DOMINANT","JAZZ IMPROVISATION",8,{0,2,4,5,7,9,10,11}},
    {"HAR_M","HARMONIC MAJOR","EXOTIC AND CLASSICAL",7,{0,2,4,5,7,8,11}},
    {"HUN_M","HUNGARIAN MINOR","EXOTIC AND DRAMATIC",7,{0,2,3,6,7,8,11}},
};

#endif