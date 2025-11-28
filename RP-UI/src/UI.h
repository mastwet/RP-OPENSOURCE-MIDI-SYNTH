#ifndef RPUILIBRE_UI_H
#define RPUILIBRE_UI_H

#define DISPLAY_SDA 21
#define DISPLAY_SCL 22
#define BTN0 4
#define BTN1 5
#define BTN2 18
#define SPEED 4 // factor of 16


#include <U8g2lib.h>

enum UIIndex {
    M_LOGO = 0,
    M_SELECT = 1,
    M_OPTIONS = 2,
    M_OPTIONS_SELECT = 3,
    M_ABOUT = 4,
};


struct SELECT_LIST {
    const char *select;
};


enum UIState {
    S_NONE,
    S_DISAPPEAR,
};

struct KEY {
    bool val;
    bool last_val;
};


struct KEY_MSG {
    uint8_t id;
    bool pressed;
};


class UI {
public:
    UI();

    ~UI();

    void init();

    void update();

    void disappear();


private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2;
    uint8_t *buf_ptr;
    uint16_t buf_len;
    UIIndex ui_index;
    UIState ui_state;
    uint8_t disappear_step = 1;


    uint8_t x; // x position
    int16_t y, y_trg;// target value and current value
    uint8_t line_y, line_y_trg;// line position
    uint8_t box_width, box_width_trg;// box width
    int16_t box_y, box_y_trg;// box current value and target value
    int8_t ui_select;//current selected column

    // option screen variables
    uint8_t pid_line_y, pid_line_y_trg;//line position
    uint8_t pid_box_width, pid_box_width_trg;//box width
    int16_t pid_box_y, pid_box_y_trg;//box current value and target value
    int8_t pid_select;//current selected column

    const float option_max = 10.00;// Max value of option
    float default_option[3] = {9.97, 0.2, 0.01}; // default options for option screen



    /** Private functions */
    static void key_init(); // init key

    static bool get_key_val(uint8_t ch); // get key value

    static void key_scan(); // scan key

    static bool move(int16_t *a, const int16_t *a_trg);

    bool move_width(uint8_t *a, const uint8_t *a_trg, uint8_t select, uint8_t id);

    static bool move_bar(uint8_t *a, const uint8_t *a_trg);

    void select_ui_show(); // select screen

    void select_proc(); // select screen process

    void logo_ui_show(); // logo screen

    void logo_proc(); // logo screen process

    void about_ui_show(); // about screen

    void about_proc(); // about screen process

    void option_ui_show(); // option screen

    void option_proc(); // option screen process

    void option_edit_ui_show(); // option edit screen

    void option_edit_proc(); // option edit screen process

};

#endif //RPUILIBRE_UI_H
