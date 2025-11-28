#include "pico/stdlib.h"
#include <stdio.h>
#include <array>

class HC74HC165 {
private:
    uint8_t pl_pin;
    uint8_t clk_pin;
    uint8_t out_pin;

    uint8_t shift_in() {
        uint8_t value = 0;
        for(int i = 0; i < 8; ++i) {
            gpio_put(clk_pin, 1);
            sleep_us(1);
            value |= gpio_get(out_pin) << (7 - i); // MSB优先
            gpio_put(clk_pin, 0);
            sleep_us(1);
        }
        return value;
    }

public:
    HC74HC165(uint8_t pl, uint8_t clk, uint8_t out) 
        : pl_pin(pl), clk_pin(clk), out_pin(out) 
    {
        gpio_init(pl_pin);
        gpio_set_dir(pl_pin, GPIO_OUT);
        gpio_put(pl_pin, 1);

        gpio_init(clk_pin);
        gpio_set_dir(clk_pin, GPIO_OUT);
        gpio_put(clk_pin, 0);

        gpio_init(out_pin);
        gpio_set_dir(out_pin, GPIO_IN);
    }

    std::array<uint8_t, 3> read_cascaded() {
        std::array<uint8_t, 3> data;
        
        // 触发并行加载
        gpio_put(pl_pin, 0);
        sleep_us(1);
        gpio_put(pl_pin, 1);

        // 级联读取（最后级芯片数据在前）
        for(int i = 0; i < 3; ++i) {
            data[2 - i] = shift_in();
        }
        
        return data;
    }
};

int test_165() {
    stdio_init_all();
    HC74HC165 shifter(10, 11, 12); // PL=10, CLK=11, OUT=12
    
    while(true) {
        auto data = shifter.read_cascaded();
        printf("Chip1:0x%02X  Chip2:0x%02X  Chip3:0x%02X\n",
               data[0], data[1], data[2]);
        sleep_ms(1000);
    }
    return 0;
}
