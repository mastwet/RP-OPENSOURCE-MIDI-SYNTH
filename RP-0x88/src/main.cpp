# include <Arduino.h>
#include "at24c.h"
#include <USBComposite.h>
#include <Versatile_RotaryEncoder.h>
#include <LiquidCrystal.h>
#include "Wire.h"


const int rs = PA8, en = PC1, d4 = PB15, d5 = PB12, d6 = PB13, d7 = PB10;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

USBCompositeSerial CompositeSerial;

// SET READING PINS ACCORDINGLY TO YOUR ENCODER TO BOARD CONNECTIONS
// Set here your encoder reading pins (Ex.: EC11 with breakout board)
#define clk PA4  // (A3)
#define dt PA5   // (A2)
#define sw PA6   // (A4)

//映射正确的KNOB
int knob_map[8] = {1,2,3,4,6,8,5,7}; 

// 定义编码器引脚
const int encoderPinA1 = PB8; // 编码器A相连接的引脚1
const int encoderPinB1 = PB9; // 编码器B相连接的引脚

const int encoderPinA2 = PC2; // 编码器A相连接的引脚2
const int encoderPinB2 = PC3; // 编码器B相连接的引脚

const int encoderPinA3 = PB4; // 编码器A相连接的引脚3
const int encoderPinB3 = PB5; // 编码器B相连接的引脚

const int encoderPinA4 = PC13; // 编码器A相连接的引脚4
const int encoderPinB4 = PC0; // 编码器B相连接的引脚

const int encoderPinA6 = PA3; // 编码器A相连接的引脚A10
const int encoderPinB6 = PA2; // 编码器B相连接的引脚

const int encoderPinA7 = PA4; // 编码器A相连接的引脚A11
const int encoderPinB7 = PA5; // 编码器B相连接的引脚

const int encoderPinA8 = PA6; // 编码器A相连接的引脚A12
const int encoderPinB8 = PA7; // 编码器B相连接的引脚

const int encoderPinA5 = PA0; // 编码器A相连接的引脚
const int encoderPinB5 = PA1; // 编码器B相连接的引脚

const int KEY1_PIN = PC12;
const int KEY2_PIN = PC11;
const int KEY3_PIN = PC10;
const int KEY4_PIN = PB11;
const int KEY5_PIN = PC4;
const int KEY6_PIN = PC5;
const int KEY7_PIN = PB0;
const int KEY8_PIN = PB1;

bool menu_in = false;
bool bank_settings = false;
int menu_in_button_pressed = 0;
int key_db_pressed_event = 0;

#define ITEM_1 0
//#define ITEM_2 1
#define ITEM_3 1

int current_page = 0;
int current_setting_option = ITEM_1;
int current_setting_knob = 0;

// Functions prototyping to be handled on each Encoder Event
void handleRotate(int8_t rotation);
void handlePressRotate(int8_t rotation);
void handleHeldRotate(int8_t rotation);
void handlePress();
void handlePress_menu_in();
void handleDoublePress();
void handlePressRelease();
void handleLongPress();
void handleLongPressRelease();
void handlePressRotateRelease();
void handleHeldRotateRelease();

// Create a global pointer for the encoder object
Versatile_RotaryEncoder *versatile_encoder;
Versatile_RotaryEncoder *versatile_encoder1;
Versatile_RotaryEncoder *versatile_encoder2;
Versatile_RotaryEncoder *versatile_encoder3;
Versatile_RotaryEncoder *versatile_encoder4;
Versatile_RotaryEncoder *versatile_encoder5;
Versatile_RotaryEncoder *versatile_encoder6;
Versatile_RotaryEncoder *versatile_encoder7;

int key_pressed_event = 0;


int encoder_channel_num[8] = {10,20,30,40,10,22,33,44};

int encoder_direct = 0;

//管理encoder的值
int encoder_value[8][8] = {0};

typedef struct {
    int8_t cc_channel;
    int8_t cc_num;
    int8_t cc_value;
} encoder_cc_setting_t;


encoder_cc_setting_t present[8][8] = {
    // 每个元素都是一个 encoder_cc_setting_t 结构体
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}
};

// void initialize_encoder_values() {
//     for (int i = 0; i < 8; i++) {
//         for (int j = 0; j < 8; j++) {
//             encoder_value[i][j] = num[i][j];
//         }
//     }
// }


// 从EEPROM写入结构体数组
void writeEEPROM() {
    uint16_t address = 0; // EEPROM起始地址
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            at24cxx_write(0x50, address, (const uint8_t*)&present[i][j], sizeof(encoder_cc_setting_t));
            address += sizeof(encoder_cc_setting_t); // 移动到下一个结构体的位置
            delay(5); // 等待EEPROM完成写操作
        }
    }
}

// 从EEPROM读取结构体数组
void readEEPROM() {
    uint16_t address = 0; // EEPROM起始地址
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            at24cxx_read(0x50, address, (uint8_t*)&present[i][j], sizeof(encoder_cc_setting_t));
            address += sizeof(encoder_cc_setting_t); // 移动到下一个结构体的位置
        }
    }
}

USBMIDI midi;

void setup() {
    //debug
    //CompositeSerial.begin(9600);

    //MIDIBASE
    USBComposite.setProductId(0x0031);
    midi.begin();

    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Hello!");
    lcd.setCursor(0, 1);
    lcd.print("RapidPulze 0x88!");

    Wire.begin();
    delay(500);
    //clearEEPROM(0x50, 1024);
    readEEPROM();

    // Initialize each encoder with its corresponding pins
    versatile_encoder = new Versatile_RotaryEncoder(encoderPinA1, encoderPinB1, KEY1_PIN);
    versatile_encoder1 = new Versatile_RotaryEncoder(encoderPinA2, encoderPinB2, KEY2_PIN);
    versatile_encoder2 = new Versatile_RotaryEncoder(encoderPinA3, encoderPinB3, KEY3_PIN);
    versatile_encoder3 = new Versatile_RotaryEncoder(encoderPinA4, encoderPinB4, KEY4_PIN);
    versatile_encoder4 = new Versatile_RotaryEncoder(encoderPinA5, encoderPinB5, KEY5_PIN);
    versatile_encoder5 = new Versatile_RotaryEncoder(encoderPinA6, encoderPinB6, KEY6_PIN);
    versatile_encoder6 = new Versatile_RotaryEncoder(encoderPinA7, encoderPinB7, KEY7_PIN);
    versatile_encoder7 = new Versatile_RotaryEncoder(encoderPinA8, encoderPinB8, KEY8_PIN);

    // Load to the encoder all nedded handle functions here (up to 9 functions)
    versatile_encoder->setHandleRotate(handleRotate);
    versatile_encoder->setHandlePress(handlePress_menu_in);
    versatile_encoder->setHandlePressRotate(handlePressRotate);
    versatile_encoder->setHandleDoublePress(handleDoublePress);
    versatile_encoder->setHandleLongPress(handleLongPress);

    versatile_encoder1->setHandleRotate(handleRotate);
    versatile_encoder1->setHandlePress(handlePress);
    versatile_encoder1->setHandlePressRotate(handlePressRotate);
    versatile_encoder1->setHandleDoublePress(handleDoublePress);

    versatile_encoder2->setHandleRotate(handleRotate);
    versatile_encoder2->setHandlePress(handlePress);
    versatile_encoder2->setHandlePressRotate(handlePressRotate);
    versatile_encoder2->setHandleDoublePress(handleDoublePress);

    versatile_encoder3->setHandleRotate(handleRotate);
    versatile_encoder3->setHandlePress(handlePress);
    versatile_encoder3->setHandlePressRotate(handlePressRotate);
    versatile_encoder3->setHandleDoublePress(handleDoublePress);

    versatile_encoder4->setHandleRotate(handleRotate);
    versatile_encoder4->setHandlePress(handlePress);
    versatile_encoder4->setHandlePressRotate(handlePressRotate);
    versatile_encoder4->setHandleDoublePress(handleDoublePress);

    versatile_encoder5->setHandleRotate(handleRotate);
    versatile_encoder5->setHandlePress(handlePress);
    versatile_encoder5->setHandlePressRotate(handlePressRotate);
    versatile_encoder5->setHandleDoublePress(handleDoublePress);

    versatile_encoder6->setHandleRotate(handleRotate);
    versatile_encoder6->setHandlePress(handlePress);
    versatile_encoder6->setHandlePressRotate(handlePressRotate);
    versatile_encoder6->setHandleDoublePress(handleDoublePress);

    versatile_encoder7->setHandleRotate(handleRotate);
    versatile_encoder7->setHandlePress(handlePress);
    versatile_encoder7->setHandlePressRotate(handlePressRotate);
    versatile_encoder7->setHandleDoublePress(handleDoublePress);

    //versatile_encoder->setHandleHeldRotate(handleHeldRotate);
    // versatile_encoder->setHandleDoublePress(handleDoublePress);
    // //versatile_encoder->setHandleDoublePress(nullptr); // Disables Double Press
    // versatile_encoder->setHandlePressRelease(handlePressRelease);
    // versatile_encoder->setHandleLongPress(handleLongPress);
    // versatile_encoder->setHandleLongPressRelease(handleLongPressRelease);
    // versatile_encoder->setHandlePressRotateRelease(handlePressRotateRelease);
    // versatile_encoder->setHandleHeldRotateRelease(handleHeldRotateRelease);

    //CompositeSerial.println("Ready!");

    // set your own defualt values (optional)
    // versatile_encoder->setInvertedSwitch(true); // inverts the switch behaviour from HIGH to LOW to LOW to HIGH
    // versatile_encoder->setReadIntervalDuration(1); // set 2ms as long press duration (default is 1ms)
    // versatile_encoder->setShortPressDuration(35); // set 35ms as short press duration (default is 50ms)
    // versatile_encoder->setLongPressDuration(550); // set 550ms as long press duration (default is 1000ms)
    // versatile_encoder->setDoublePressDuration(350); // set 350ms as double press duration (default is 250ms)

}

void output_cc_value(int number) {  // 增加了value参数及可选的channel参数
  // 发送控制变更消息
  // 参数：通道 (0-15), 控制号 (0-127), 值 (0-127)
  midi.sendControlChange( 
        present[current_page][current_setting_knob].cc_channel,
        present[current_page][current_setting_knob].cc_value,
        //present里面的CC只在设定的时候加载默认值
        encoder_value[current_page][number]
  );
}

void output_value(int number, int direct) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("N:");

  // 格式化数字为三位数字符串
  char buffer[4];
  sprintf(buffer, "%3d", number);
  lcd.print(buffer);

  lcd.setCursor(6, 0);
  lcd.print("CH:");
  lcd.print(present[current_page][number].cc_channel);
  lcd.setCursor(13, 0);
  lcd.print("P");
  lcd.print(current_page);
  lcd.setCursor(0, 1);

  // 更新 encoder_value[number],步进我不想改，放在二期需求里面
  if (direct == 1) {
    if (encoder_value[current_page][number] < 127) {
      encoder_value[current_page][number] = encoder_value[current_page][number] + 1*4;
    }
  } else if (direct == -1) {
    if (encoder_value[current_page][number] > 0) {
      encoder_value[current_page][number] = encoder_value[current_page][number] - 1*4;
    }
  }

  // 显示 encoder_value[number]
  sprintf(buffer, "%3d", encoder_value[current_page][number]);
  lcd.print(buffer);

  // 在第二行第五个格子开始显示进度条
  int progressPosition = 5;
  lcd.setCursor(progressPosition, 1);

  // 计算进度条的长度（最大长度为 10 个格子）
  int progressLength = (encoder_value[current_page][number] * 10) / 127;

  // 填充进度条
  for (int i = 0; i < 10; i++) {
    if (i < progressLength) {
      lcd.print((char)255);  // 用 '#' 表示进度条已填充部分
    } else {
      lcd.print(" ");  // 用空格表示未填充部分
    }
  }

    output_cc_value(number);

}

void show_menu_ui() {
    // 清空 LCD 显示

    if(bank_settings == false){
    
    // 第一行显示：CC值，CC控制器编号，CC通道
    lcd.setCursor(0, 0);
    if (current_setting_option == 0) {
        lcd.print("<CTRL>");
    } else {
        lcd.print("CTRL ");
    }
    // if (current_setting_option == 1) {
    //     lcd.print("<NUM>");
    // } else {
    //     lcd.print("NUM ");
    // }
    if (current_setting_option == 1) {
        lcd.print("<CHAN>");
    } else {
        lcd.print("CHAN ");
    }

    // 获取当前选中的旋钮参数索引

    // 第二行显示：CC值，CC控制器编号，CC通道对应的 VALUE
    // 显示 CC 值
    lcd.setCursor(0, 1);
    int ccValue = present[current_page][current_setting_knob].cc_value;
    char buffer[4]; // Buffer to hold the formatted string
    sprintf(buffer, "%3d", ccValue); // Format the number to take up 3 spaces, padding with spaces if necessary
    lcd.print(buffer);

    // // 显示 CC 控制器编号
    //     lcd.setCursor(6, 1);
    //     int ccNum = present[current_page][current_setting_knob].cc_num;
    //     sprintf(buffer, "%3d", ccNum);
    //     lcd.print(buffer);

    // 显示 CC 通道
        lcd.setCursor(6, 1);
        int ccChannel = present[current_page][current_setting_knob].cc_channel;
        sprintf(buffer, "%3d", ccChannel);
        lcd.print(buffer);

        // Add "K2" at the right end of the first line
        lcd.setCursor(14, 0);
        lcd.print("K");
        lcd.setCursor(15, 0);
        lcd.print(current_setting_knob);

    // Add "P4" at the right end of the second line
        lcd.setCursor(14, 1);
        lcd.print("P");
        lcd.setCursor(15, 1);
        lcd.print(current_page);
    }
    else{
    // Flip page logic
    //lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Bank Settings            "); // Placeholder text for bank settings
        lcd.setCursor(0, 1);
        lcd.print("Page:");
        lcd.print(current_page + 1); // Display current page number starting from 1
        lcd.print("           "); // Display current page number starting from 1
    }
}

void set_value(int number,int direct){
    if(bank_settings == false){
    if(current_setting_knob != number){
        current_setting_knob = number;
    }
    switch(current_setting_option){
        case ITEM_1:
        if(direct == 1){
            //不是encoder value
            if(present[current_page][current_setting_knob].cc_value < 127 ){
                 present[current_page][current_setting_knob].cc_value++;
            }
        }
        else if(direct == -1){
            if(present[current_page][current_setting_knob].cc_value > 0 ){
                present[current_page][current_setting_knob].cc_value--;
            }
        }
        break;
        // case ITEM_2:
        //     if(direct == 1){
        //         if(present[current_page][current_setting_knob].cc_num < 127 ){
        //             present[current_page][current_setting_knob].cc_num++;
        //         }
        //     }
        //     else if(direct == -1){
        //         if(present[current_page][current_setting_knob].cc_num > 0 ){
        //             present[current_page][current_setting_knob].cc_num--;
        //         }
        //     }
        //     break;
        case ITEM_3:
            if(direct == 1){
                if(present[current_page][current_setting_knob].cc_channel < 15 ){
                    present[current_page][current_setting_knob].cc_channel++;
                }
            }
            else if(direct == -1){
                if(present[current_page][current_setting_knob].cc_channel > 0 ){
                    present[current_page][current_setting_knob].cc_channel--;
                }
            }
            break;
        default:
            break;
    }
    }
    else{
        if(direct == 1){
            if(current_page < 7){
                current_page++;
            }
        }
        else if(direct == -1){
            if(current_page > 0){
                current_page--;
            }
        }
    }
}

void loop() {

    if(menu_in){
        show_menu_ui();
    }

    // Do the encoder reading and processing
    if (versatile_encoder->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
            output_value(0,encoder_direct); 
        }
        else{
        set_value(0,encoder_direct);
                
        //如果按键按下，进入改变page模式？
        if(key_pressed_event == 1){
            //则选中的编码器发生变化
            //current_setting_knob = 0;
            set_value(0,encoder_direct); 
            //key_pressed_event = 0;
        }
        }
    encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder1->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
            output_value(1,encoder_direct);
        }
        else{
            set_value(1,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder2->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(2,encoder_direct);
        }
        else{
            set_value(2,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder3->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(3,encoder_direct);
        }
        else{
            set_value(3,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder4->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(4,encoder_direct);
        }
        else{
            set_value(4,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder5->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(5,encoder_direct);
        }
        else{
            set_value(5,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder6->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(6,encoder_direct);
        }
        else{
            set_value(6,encoder_direct); 
        }
            encoder_direct = 0;
    }
    // Do the encoder reading and processing
    if (versatile_encoder7->ReadEncoder()) {
        // Do something here whenever an encoder action is read
        if(!menu_in){
        output_value(7,encoder_direct);
        }
        else{
            set_value(7,encoder_direct); 
        }
            encoder_direct = 0;
    }

            if(key_db_pressed_event == 1){
                if(current_setting_option <1){
                    current_setting_option+=1;    
                }
                else{
                    current_setting_option = 0;
                }
                
                key_db_pressed_event = 0;
                if(menu_in_button_pressed>0) menu_in_button_pressed = 0;
            }    
    
}


// Implement your functions here accordingly to your needs

void handleRotate(int8_t rotation) {
	//CompositeSerial.print("#1 Rotated: ");
    if (rotation > 0){
	//CompositeSerial.println("Right");
      encoder_direct = 1;
    }
    else{
	//CompositeSerial.println("Left");
      encoder_direct = -1;
    }
}

void handlePressRotate(int8_t rotation) {
	// CompositeSerial.print("#2 Pressed and rotated: ");
    // if (rotation > 0)
	//     CompositeSerial.println("Right");
    // else
	//     CompositeSerial.println("Left");
}

void handleHeldRotate(int8_t rotation) {
	// CompositeSerial.print("#3 Held and rotated: ");
    // if (rotation > 0)
	//     CompositeSerial.println("Right");
    // else
	//     CompositeSerial.println("Left");
}

void handlePress() {
    key_pressed_event = 1;
	//CompositeSerial.println("#4.1 Pressed");
}
 
void handlePress_menu_in() {
    menu_in_button_pressed += 1;
    if(menu_in_button_pressed >= 5){
        lcd.clear();
        //退出前保存配置
        if(menu_in == true){
            lcd.setCursor(0,0);
            lcd.print("Saving Presets");
            lcd.setCursor(0,1);
            lcd.print("please wait...");
            writeEEPROM();
            lcd.clear();
        }
        menu_in = !menu_in;
        menu_in_button_pressed = 0;
    }

	//CompositeSerial.println("#4.1 Pressed");
}

void handleDoublePress() {
	//CompositeSerial.println("#4.2 Double Pressed");
    key_db_pressed_event = 1;
}

void handlePressRelease() {
	//CompositeSerial.println("#5 Press released");
}

void handleLongPress() {
	//CompositeSerial.println("#6 Long pressed");
    if(menu_in){
        bank_settings = !bank_settings;
    }
}

void handleLongPressRelease() {
	//CompositeSerial.println("#7 Long press released");
}

void handlePressRotateRelease() {
	//CompositeSerial.println("#8 Press rotate released");
}

void handleHeldRotateRelease() {
	//CompositeSerial.println("#9 Held rotate released");
}