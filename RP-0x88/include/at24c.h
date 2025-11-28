#include <Arduino.h>

#ifndef AT24CXX_H
#define AT24CXX_H

#define AT24C16

#ifdef AT24C02
#define PAGE_SIZE   (8)
#endif
#if defined (AT24C04) || defined (AT24C08) || defined (AT24C16)
#define PAGE_SIZE   (16)
#endif
#if defined(AT24C32) || defined(AT24C64)
#define PAGE_SIZE   (32)
#endif


void at24cxx_init(void);
int at24cxx_read_byte(int slave, int addr);
void at24cxx_write_byte(int slave, int addr, int data);
int at24cxx_write(int slave, int addr, const uint8_t *buf, int buflen);
int at24cxx_read(int slave, int addr, uint8_t *buf, int buflen);
void clearEEPROM(uint8_t slave, uint16_t size);

#endif /*AT24C16_H*/