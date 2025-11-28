#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

class RotaryEncoder {
public:
    enum class Direction {
        Stopped,
        Left,
        Right
    };

    using Callback = void (*)(Direction);

    RotaryEncoder(uint clk_pin, 
                 uint dt_pin,
                 uint32_t read_interval_ms,
                 Callback callback)
        : clk_pin_(clk_pin),
          dt_pin_(dt_pin),
          read_interval_us_(read_interval_ms * 1000),
          callback_(callback) {
        // 初始化GPIO
        gpio_init(clk_pin_);
        gpio_init(dt_pin_);
        gpio_set_dir(clk_pin_, GPIO_IN);
        gpio_set_dir(dt_pin_, GPIO_IN);
        gpio_pull_up(clk_pin_);
        gpio_pull_up(dt_pin_);
        
        // 初始化状态
        rotary_bits_ = 0xFF;
        last_read_ = get_absolute_time();
    }

    void Update() {
        if (absolute_time_diff_us(last_read_, get_absolute_time()) < read_interval_us_)
            return;
        
        last_read_ = get_absolute_time();
        uint8_t current_state = (gpio_get(clk_pin_) << 1) | gpio_get(dt_pin_);
        
        if ((rotary_bits_ & 0x03) != (current_state & 0x03)) {
            rotary_bits_ = (rotary_bits_ << 2) | (current_state & 0x03);
            
            Direction dir = Direction::Stopped;
            if (rotary_bits_ == 0x4B) {
                dir = Direction::Left;
            } else if (rotary_bits_ == 0x87) {
                dir = Direction::Right;
            }
            
            if (dir != Direction::Stopped && callback_) {
                callback_(dir);
            }
        }
    }

private:
    uint clk_pin_;
    uint dt_pin_;
    uint8_t rotary_bits_;
    absolute_time_t last_read_;
    uint32_t read_interval_us_;
    Callback callback_;
};

/*********************** 使用示例 **************************/
void OnRotate(RotaryEncoder::Direction dir) {
    printf(dir == RotaryEncoder::Direction::Left ? "Left\n" : "Right\n");
}

int test_encoder() {
    stdio_init_all();
    printf("Pico Rotary Encoder Test\n");
    
    RotaryEncoder encoder(20, 21, 5, OnRotate);
    
    while(true) {
        encoder.Update();
        sleep_ms(1);
    }
}