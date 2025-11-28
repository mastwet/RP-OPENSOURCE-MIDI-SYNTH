#include "at24c.h"
#include <Arduino.h>
#include <Wire.h>
void at24cxx_init(void)
{
    Wire.begin();
}

static void at24cxx_wait(int slave)
{
    int result = 0;
    do
    {
        Wire.beginTransmission(slave);
        result = Wire.endTransmission();
    } while (result != 0);
}

void at24cxx_write_byte(int slave, int addr, int data)
{
    at24cxx_wait(slave);
    Wire.beginTransmission(slave);
    Wire.write(addr);  /*发送写地址*/
    Wire.write(data);  /*写数据*/
    Wire.endTransmission();
}

int at24cxx_read_byte(int slave, int addr)
{
    int  ret = 0xFF;
    at24cxx_wait(slave);
    Wire.beginTransmission(slave);
    Wire.write(addr);  /*发送读地址*/
    Wire.endTransmission();   
    /*读数据*/
    Wire.requestFrom(slave, 1);
    ret = Wire.read();
    return ret;
}

int at24cxx_write(int slave, int addr, const uint8_t *buf, int buflen)
{
    int left_len = buflen;
    int send_len = PAGE_SIZE-(addr%PAGE_SIZE);
    send_len = (left_len>send_len) ? send_len:left_len;
    while(left_len)
    {
        at24cxx_wait(slave);
        Wire.beginTransmission(slave);
        Wire.write(addr);  /*发送写地址*/
        Wire.write(const_cast<uint8_t*>(buf), send_len);  /*写数据*/
        Wire.endTransmission();
        left_len -= send_len;
        addr += send_len;
        buf += send_len;
        send_len = (left_len>PAGE_SIZE) ? PAGE_SIZE:left_len;
    }
    return buflen;
}

int at24cxx_read(int slave, int addr, uint8_t *buf, int buflen)
{
    int left_len = buflen;
    int get_len = 0;
    while(left_len)
    {
        if(left_len > 32)
        {
            get_len = 32;
        }
        else
        {
            get_len = left_len;
        }
        at24cxx_wait(slave);
        Wire.beginTransmission(slave);
        Wire.write(addr);  /*发送读地址*/
        Wire.endTransmission();   
        /*读数据*/
        Wire.requestFrom(slave, get_len);
        for(int i=0; i<get_len; i++)
        {
            buf[i] = Wire.read();
        }
        left_len -= get_len;
        buf += get_len;
        addr += get_len;
    }
    return buflen;
}

// 清零EEPROM
void clearEEPROM(uint8_t slave, uint16_t size) {
    uint8_t zeroData[PAGE_SIZE];
    memset(zeroData, 0, PAGE_SIZE);

    for (uint16_t addr = 0; addr < size; addr += PAGE_SIZE) {
        uint16_t len = min(PAGE_SIZE, size - addr);
        at24cxx_write(slave, addr, zeroData, len);
        delay(5); // 等待EEPROM完成写操作
    }
}