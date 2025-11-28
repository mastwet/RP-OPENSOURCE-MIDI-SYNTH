#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/sync.h"
#include "hardware/gpio.h"

class ButtonScanner {
private:
    uint* buttons;
    int count;
    void (*callback)(uint gpio);

public:
    ButtonScanner(uint buttons[], int count) {
        this->buttons = buttons;
        this->count = count;
        this->callback = nullptr;
        
        // 初始化所有按键GPIO
        for (int i = 0; i < count; i++) {
            uint gpio_pin = buttons[i];
            gpio_init(gpio_pin);
            gpio_set_dir(gpio_pin, GPIO_IN);
            gpio_pull_up(gpio_pin);
        }
    }

    ~ButtonScanner() {
        delete[] buttons;
    }

    void set_callback(void (*cb)(uint gpio)) {
        this->callback = cb;
    }

    void scan_buttons() {
        for (int i = 0; i < count; i++) {
            uint gpio_pin = buttons[i];
            if (!gpio_get(gpio_pin)) {
                // 简单的去抖动处理
                busy_wait_ms(20);
                if (!gpio_get(gpio_pin) && callback != nullptr) {
                    callback(gpio_pin);
                }
            }
        }
    }
};

int button_test() {
    //stdio_init_all();
    uint buttons[] = {2, 3, 4, 5, 6};
    ButtonScanner scanner(buttons, 5);
    
    // 设置回调函数
    scanner.set_callback([](uint gpio) {
        printf("Button %d pressed!\n", gpio);
    });
    
    while (1) {
        scanner.scan_buttons();
        tight_loop_contents();
    }
}