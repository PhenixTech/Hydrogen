#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "bitmap.h"
#include "drivers/pcf8563.h"

#define LED_PIN   GPIO_Pin_2
#define LED_PORT  GPIOD

typedef struct { uint16_t on; uint16_t off; uint8_t count; } Pattern;
// static const Pattern PAT_NOT_FOUND = { 800, 800, 1 };
static const Pattern PAT_INIT_FAIL = { 100, 100, 2 };
static const Pattern PAT_OK        = {  80,  80, 1 };


typedef struct {
    uint8_t date_page,  date_col;
    uint8_t hour_page,  hour_col;
    uint8_t min_page,   min_col;
    uint8_t sec_page,   sec_col;
    const uint8_t *bmp;
    uint8_t bmp_page,  bmp_col;
    uint8_t bmp_w,  bmp_h;
    const uint8_t *colon1;
    uint8_t colon1_page, colon1_col;
    const uint8_t *colon2;
    uint8_t colon2_page, colon2_col;
    bool show_sec;
} UI_Theme;

void u8_to_str(uint8_t val, char *buf);
void u16_to_str(uint16_t val, char *buf);

void Draw_Clock(RTC_Time *now, RTC_Time *prev);
void Draw_Inputs(void);

void LED_Init(void);
void LED_Set(uint8_t on);
void Blink(const Pattern *p);

extern uint8_t ismenu;
extern bool devmode;

#endif

