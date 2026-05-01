#include "ch32v00X.h"
#include "i2c.h"
#include "pcf8563.h"
#include <stdbool.h>

#define PCF8563_ADDR  0x51

bool VL = 0;

static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

uint8_t PCF8563_Init(void)
{
    if (I2C_Probe(PCF8563_ADDR) != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x00, 0x00) != 0) return 2;
    if (I2C_WriteReg(PCF8563_ADDR, 0x01, 0x00) != 0) return 2;
    uint8_t val = 0xFF;
    if (I2C_ReadReg(PCF8563_ADDR, 0x00, &val) != 0) return 3;
    if (val != 0x00) return 3;
    return 0;
}

uint8_t PCF8563_SetTime(RTC_Time *t)
{
    if (I2C_WriteReg(PCF8563_ADDR, 0x02, dec2bcd(t->sec))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x03, dec2bcd(t->min))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x04, dec2bcd(t->hour))        != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x05, dec2bcd(t->day))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x07, dec2bcd(t->month))       != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x08, dec2bcd(t->year - 2000)) != 0) return 1;
    return 0;
}

uint8_t PCF8563_GetTime(RTC_Time *t)
{
    uint8_t buf[7];
    if (I2C_ReadBurst(PCF8563_ADDR, 0x02, buf, 7) != 0) return 1;
    t->sec   = bcd2dec(buf[0] & 0x7F);
    t->min   = bcd2dec(buf[1] & 0x7F);
    t->hour  = bcd2dec(buf[2] & 0x3F);
    t->day   = bcd2dec(buf[3] & 0x3F);
    t->month = bcd2dec(buf[5] & 0x1F);
    t->year  = bcd2dec(buf[6]) + 2000;
    return 0;
}



bool checkVL(void) 
{
    uint8_t sec_reg = 0;
    if (I2C_ReadReg(PCF8563_ADDR, 0x02, &sec_reg) == 0 && (sec_reg & 0x80)) {
        uint8_t VL = 1;
        return true;
    }
    return false;
}