#include "pico/stdlib.h"  // 包含PICO SDK的标准库头文件
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "config.h"
#include "BUTTONS.h"
#include "74HC165.h"
#include "ENCODER.h"

void device_init();

extern uint buttons;

ButtonScanner *scanner;
HC74HC165 *shifter;
RotaryEncoder *encoder;
ssd1306_t disp;

int main() {
    stdio_init_all();

    device_init();
    
    while (true) {
        
    }
}

void device_init() {
    scanner = new ButtonScanner(buttons, 5);
    scanner->set_callback([] (uint gpio) {
        printf("Button %d pressed!\n", gpio);
    });

    shifter = new HC74HC165(HC165_LATCH_PIN, HC165_CLOCK_PIN, HC165_DATA_PIN);
    encoder = new RotaryEncoder(20, 21, 5, OnRotate);

    i2c_init(i2c0, 400000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c0);
    ssd1306_clear(&disp);

}