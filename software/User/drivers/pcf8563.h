#ifndef PCF8563_H
#define PCF8563_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t sec, min, hour, day, month;
    uint16_t year;
} RTC_Time;

uint8_t PCF8563_SetTime(RTC_Time *t);
uint8_t PCF8563_GetTime(RTC_Time *t);
uint8_t PCF8563_Init(void);
void PCF8563_ClearVL(void);
bool checkVL(void);

extern bool VL;

#endif