#ifndef NES_NOISE_H
#define NES_NOISE_H

#include <stdio.h>

const uint32_t noise_freq_table[2][16]
 = {
    {440, 880, 1762, 2349, 3523, 4710, 7046, 8860, 11186, 13983, 18642, 27965, 55930, 111861, 223722, 447443},
    {5, 10, 19, 25, 38, 51, 76, 95, 120, 150, 201, 301, 601, 1203, 2406, 4811}
    // {350, 700, 1330, 1750, 2660, 3570, 5320, 6650, 8400, 10500, 14070, 21070, 42070, 84210, 168420, 336770}
};

int16_t nes_noise_make(uint8_t period, bool mode, uint32_t smp_rate) {
    static uint16_t shift_register = 1;
    static uint32_t period_counter = 0;

    uint32_t freq = noise_freq_table[mode][period];

    float oversampling_factor = freq > smp_rate ? freq / (float)smp_rate : 1;
    int32_t sum_output = 0;

    for (uint8_t i = 0; i < oversampling_factor; i++) {
        if (period_counter == 0) {
            period_counter = (freq > smp_rate ? freq : smp_rate) / freq;
            uint16_t feedback_bit = mode ? 6 : 1;
            uint16_t feedback = ((shift_register & 1) ^ ((shift_register >> feedback_bit) & 1)) << 14;
            shift_register = (shift_register >> 1) | feedback;
        }
        period_counter--;
        sum_output += (shift_register & 1) ? 64 : -64;
    }
    return sum_output / oversampling_factor;
}

/*
int16_t nes_noise_make(uint8_t period, bool mode, uint16_t smp_rate) {
    static uint16_t shift_register = 1;  // 15-bit wide shift register initialized to 1
    uint16_t feedback_bit = mode ? 6 : 1;  // Choose feedback bit based on mode
    bool feedback;

    // Period should be within the range 0-15 to avoid out-of-bounds access
    if (period > 15) period = 15;

    // Use period to determine the operation frequency from the table
    uint32_t frequency = noise_freq_table[mode][period];

    // Calculate feedback: XOR of bit 0 and the selected feedback bit
    feedback = (shift_register & 1) ^ ((shift_register >> feedback_bit) & 1);

    // Shift the register right by one bit
    shift_register >>= 1;

    // Set bit 14 to the feedback
    if (feedback) {
        shift_register |= (1 << 14);
    }

    // Check bit 0 to determine output value
    if (shift_register & 1) {
        return -64;
    } else {
        return 64;
    }
}
*/
#endif // NES_NOISE_H