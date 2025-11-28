#include "UI.h"
#include "bitmaps.h"


SELECT_LIST pid[]
        {
                {"-Option 1"},
                {"-Option 2"},
                {"-Option 3"},
                {"Return"},
        };

uint8_t pid_num = sizeof(pid) / sizeof(SELECT_LIST);// option menu choice count

SELECT_LIST list[]
        {
                {"MainUI"},
                {"+Options"},
                {"{ About }"},
        };

uint8_t list_num = sizeof(list) / sizeof(SELECT_LIST); // main menu choice count
uint8_t single_line_length = 63 / list_num;
uint8_t total_line_length = single_line_length * list_num + 1;

KEY_MSG key_msg = {0};
KEY key[3] = {false};

UI::UI() {
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, DISPLAY_SCL, DISPLAY_SDA);
}

UI::~UI() {
    delete u8g2;
}

void UI::init() {
    Serial.begin(9600);
    pinMode(BTN0, INPUT_PULLUP);
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    key_init();
    u8g2->setBusClock(800000);
    u8g2->begin();
    u8g2->setFont(u8g2_font_wqy12_t_chinese1);
    buf_ptr = u8g2->getBufferPtr();
    buf_len = 8 * u8g2->getBufferTileHeight() * u8g2->getBufferTileWidth();

    x = 4;
    y = y_trg = 0;
    line_y = line_y_trg = 1;
    pid_line_y = pid_line_y_trg = 1;
    ui_select = pid_select = 0;
    box_width = box_width_trg = u8g2->getStrWidth(list[ui_select].select) + x * 2;
    pid_box_width = pid_box_width_trg = u8g2->getStrWidth(pid[pid_select].select) + x * 2;

    ui_index = M_LOGO;
    ui_state = S_NONE;

}

void UI::update() {

    // update buttons
    key_scan();

    switch (ui_state) {
        case S_NONE:
            u8g2->clearBuffer();
            switch (ui_index) {
                case M_LOGO:
                    logo_proc();
                    break;
                case M_SELECT:
                    select_proc();
                    break;
                case M_OPTIONS:
                    option_proc();
                    break;
                case M_OPTIONS_SELECT:
                    option_edit_proc();
                    break;
                case M_ABOUT:
                    about_proc();
                    break;
                default:
                    break;
            }
            break;
        case S_DISAPPEAR:
            disappear();
            break;
        default:
            break;
    }
    u8g2->sendBuffer();

}

void UI::disappear() {
    switch (disappear_step) {
        case 1:
            for (uint16_t i = 0; i < buf_len; ++i) {
                if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55;
            }
            break;
        case 2:
            for (uint16_t i = 0; i < buf_len; ++i) {
                if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA;
            }
            break;
        case 3:
            for (uint16_t i = 0; i < buf_len; ++i) {
                if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00;
            }
            break;
        case 4:
            for (uint16_t i = 0; i < buf_len; ++i) {
                if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00;
            }
            break;
        default:
            ui_state = S_NONE;
            disappear_step = 0;
            break;
    }
    disappear_step++;
}

void UI::key_init() {
    for (uint8_t i = 0; i < (uint8_t) (sizeof(key) / sizeof(KEY)); ++i) {
        key[i].val = key[i].last_val = get_key_val(i);
    }
}

bool UI::get_key_val(uint8_t ch) {
    switch (ch) {
        case 0:
            return digitalRead(BTN0);
        case 1:
            return digitalRead(BTN1);
        case 2:
            return digitalRead(BTN2);
        default:
            break;
    }
    return false;
}

void UI::key_scan() {
    for (uint8_t i = 0; i < (uint8_t) (sizeof(key) / sizeof(KEY)); ++i) {
        key[i].val = get_key_val(i); // read key value
        if (key[i].last_val != key[i].val)// if key value changed
        {
            key[i].last_val = key[i].val;// update last key value
            if (key[i].val == LOW) {
                key_msg.id = i;
                key_msg.pressed = true;
            }
        }
    }
}

bool UI::move_bar(uint8_t *a, const uint8_t *a_trg) {
    if (*a < *a_trg) {
        uint8_t step = 16 / SPEED;// determine the number of steps
        uint8_t width_speed = ((single_line_length % step) == 0 ? (single_line_length / step) : (
                single_line_length / step + 1));   // calculate the step length
        *a += width_speed;
        if (*a > *a_trg) *a = *a_trg;//
    } else if (*a > *a_trg) {
        uint8_t step = 16 / SPEED;// determine the number of steps
        uint8_t width_speed = ((single_line_length % step) == 0 ? (single_line_length / step) : (
                single_line_length / step + 1));   // calculate the step length
        *a -= width_speed;
        if (*a < *a_trg) *a = *a_trg;
    } else {
        return true;// arrived
    }
    return false;// not yet
}

bool UI::move_width(uint8_t *a, const uint8_t *a_trg, uint8_t select, uint8_t id) {
    uint8_t len = 0;
    if (*a < *a_trg) {
        uint8_t step = 16 / SPEED;// determine the number of steps
        if (ui_index == M_SELECT) {
            len = abs(u8g2->getStrWidth(list[select].select) -
                      u8g2->getStrWidth(list[id == 0 ? select + 1 : select - 1].select));
        } else if (ui_index == M_OPTIONS) {
            len = abs(u8g2->getStrWidth(pid[select].select) -
                      u8g2->getStrWidth(pid[id == 0 ? select + 1 : select - 1].select));
        }
        uint8_t width_speed = ((len % step) == 0 ? (len / step) : (len / step + 1));   // calculate the step length
        *a += width_speed;
        if (*a > *a_trg) *a = *a_trg;
    } else if (*a > *a_trg) {
        uint8_t step = 16 / SPEED;// determine the number of steps
        if (ui_index == M_SELECT) {
            len = abs(u8g2->getStrWidth(list[select].select) -
                      u8g2->getStrWidth(list[id == 0 ? select + 1 : select - 1].select));
        } else if (ui_index == M_OPTIONS) {
            len = abs(u8g2->getStrWidth(pid[select].select) -
                      u8g2->getStrWidth(pid[id == 0 ? select + 1 : select - 1].select));
        }
        uint8_t width_speed = ((len % step) == 0 ? (len / step) : (len / step + 1));   //计算步长
        *a -= width_speed;
        if (*a < *a_trg) *a = *a_trg;
    } else {
        return true;// arrived
    }
    return false;// not yet
}

bool UI::move(int16_t *a, const int16_t *a_trg) {
    if (*a < *a_trg) {
        *a += SPEED;
        if (*a > *a_trg) *a = *a_trg;// add not enough
    } else if (*a > *a_trg) {
        *a -= SPEED;
        if (*a < *a_trg) *a = *a_trg;// subtract not enough
    } else {
        return true;// target reached
    }
    return false;// not yet
}


/**
 * @brief Display selected UI
 */
void UI::select_ui_show() {
    move_bar(&line_y, &line_y_trg);
    move(&y, &y_trg);
    move(&box_y, &box_y_trg);
    move_width(&box_width, &box_width_trg, ui_select, key_msg.id);
    u8g2->drawVLine(126, 0, total_line_length);
    u8g2->drawPixel(125, 0);
    u8g2->drawPixel(127, 0);
    for (uint8_t i = 0; i < list_num; ++i) {
        u8g2->drawStr(x, 16 * i + y + 12, list[i].select);
        u8g2->drawPixel(125, single_line_length * (i + 1));
        u8g2->drawPixel(127, single_line_length * (i + 1));
    }
    u8g2->drawVLine(125, line_y, single_line_length - 1);
    u8g2->drawVLine(127, line_y, single_line_length - 1);
    u8g2->setDrawColor(2);
    u8g2->drawRBox(0, box_y, box_width, 16, 1);
    u8g2->setDrawColor(1);
}

void UI::select_proc() {
    if (key_msg.pressed) {
        key_msg.pressed = false;
        switch (key_msg.id) {
            case 0:
                if (ui_select < 1) break;
                ui_select -= 1;
                line_y_trg -= single_line_length;
                if (ui_select < -(y / 16)) {
                    y_trg += 16;
                } else {
                    box_y_trg -= 16;
                }

                break;
            case 1:
                if ((ui_select + 2) > (sizeof(list) / sizeof(SELECT_LIST))) break;
                ui_select += 1;
                line_y_trg += single_line_length;
                if ((ui_select + 1) > (4 - y / 16)) {
                    y_trg -= 16;
                } else {
                    box_y_trg += 16;
                }

                break;
            case 2:
                switch (ui_select) {
                    case 0:     //return
                        ui_state = S_DISAPPEAR;
                        ui_index = M_LOGO;
                        break;
                    case 1:     //pid
                        ui_state = S_DISAPPEAR;
                        ui_index = M_OPTIONS;
                        break;
                    case 3:   //about
                        ui_state = S_DISAPPEAR;
                        ui_index = M_ABOUT;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
        box_width_trg = u8g2->getStrWidth(list[ui_select].select) + x * 2;
    }
    select_ui_show();
}


/**
 * @brief Display LOGO
 */
void UI::logo_ui_show()// display logo
{
    u8g2->drawXBMP(0, 0, 128, 64, LOGO);
}

void UI::logo_proc() {
    if (key_msg.pressed) {
        key_msg.pressed = false;
        ui_state = S_DISAPPEAR;
        ui_index = M_SELECT;
    }
    logo_ui_show();
}


/**
 * @brief Display About UI
 */
void UI::about_ui_show()// about page
{

    u8g2->drawStr(2, 12, "MCU : ESP32");
    u8g2->drawStr(2, 28, "FLASH : 4MB");
    u8g2->drawStr(2, 44, "SRAM : 520KB");
    u8g2->drawStr(2, 60, "RTC SRAM : 16KB");
}

void UI::about_proc() {
    if (key_msg.pressed) {
        key_msg.pressed = false;
        ui_state = S_DISAPPEAR;
        ui_index = M_SELECT;
    }
    about_ui_show();
}


/**
 * @brief Display Options UI
 */
void UI::option_ui_show() {
    move_bar(&pid_line_y, &pid_line_y_trg);
    move(&pid_box_y, &pid_box_y_trg);
    move_width(&pid_box_width, &pid_box_width_trg, pid_select, key_msg.id);
    u8g2->drawVLine(126, 0, 61);
    u8g2->drawPixel(125, 0);
    u8g2->drawPixel(127, 0);
    for (uint8_t i = 0; i < pid_num; ++i) {
        u8g2->drawStr(x, 16 * i + 12, pid[i].select);
        u8g2->drawPixel(125, 15 * (i + 1));
        u8g2->drawPixel(127, 15 * (i + 1));
    }

    u8g2->setDrawColor(2);
    u8g2->drawRBox(0, pid_box_y, pid_box_width, 16, 1);
    u8g2->setDrawColor(1);
    u8g2->drawVLine(125, pid_line_y, 14);
    u8g2->drawVLine(127, pid_line_y, 14);
}

void UI::option_proc() {
    option_ui_show();
    if (key_msg.pressed) {
        key_msg.pressed = false;
        switch (key_msg.id) {
            case 0:
                if (pid_select != 0) {
                    pid_select -= 1;
                    pid_line_y_trg -= 15;
                    pid_box_y_trg -= 16;
                    break;
                } else {
                    break;
                }
            case 1:
                if (pid_select != 3) {
                    pid_select += 1;
                    pid_line_y_trg += 15;
                    pid_box_y_trg += 16;
                } else {
                    break;
                }
                break;
            case 2:
                if (pid_select == 3) {
                    ui_index = M_SELECT;
                    ui_state = S_DISAPPEAR;
                    pid_select = 0;
                    pid_line_y = pid_line_y_trg = 1;
                    pid_box_y = pid_box_y_trg = 0;
                    pid_box_width = pid_box_width_trg = u8g2->getStrWidth(pid[pid_select].select) + x * 2;
                } else {
                    ui_index = M_OPTIONS_SELECT;
                }
                break;
            default:
                break;
        }
        pid_box_width_trg = u8g2->getStrWidth(pid[pid_select].select) + x * 2;
    }
}


/**
 * @brief Display Options Edit UI
 */
void UI::option_edit_ui_show() {
    u8g2->drawBox(16, 16, 96, 31);
    u8g2->setDrawColor(2);
    u8g2->drawBox(17, 17, 94, 29);
    u8g2->setDrawColor(1);
    u8g2->drawFrame(18, 36, 60, 8);
    u8g2->drawBox(20, 38, (uint8_t) (default_option[pid_select] / option_max * 56), 4);

    u8g2->setCursor(22, 30);
    switch (pid_select) {
        case 0:
            u8g2->print("Editing Option 1");
            break;
        case 1:
            u8g2->print("Editing Option 2");
            break;
        case 2:
            u8g2->print("Editing Option 3");
            break;
        default:
            break;
    }

    u8g2->setCursor(81, 44);
    u8g2->print(default_option[pid_select]);

}

void UI::option_edit_proc() {
    if (key_msg.pressed) {
        key_msg.pressed = false;
        switch (key_msg.id) {
            case 0:
                if (default_option[pid_select] > 0) default_option[pid_select] -= 0.01;
                break;
            case 1:
                if (default_option[pid_select] < option_max) default_option[pid_select] += 0.01;
                break;
            case 2:
                ui_index = M_OPTIONS;
                break;
            default:
                break;
        }
    }
    option_ui_show();
    for (uint16_t i = 0; i < buf_len; ++i) {
        buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);
    }
    option_edit_ui_show();
}

