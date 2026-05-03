#include <stdbool.h>
#include "ch32v00X.h"
#include "drivers/pcf8563.h"
#include "drivers/ssd1306.h"
#include "ui.h"
#include "button.h"
#include "menu.h"
uint8_t current_theme = 0;
bool themedrawn = false;

static const UI_Theme themes[] = {
    { .hour_page = 2, .hour_col = 20, .min_page = 2,  .min_col = 44, .sec_page = 2, .sec_col = 68, .date_page = 0, .date_col = 14, .colon1_page = 2, .colon1_col = 38, .colon2_page = 2, .colon2_col = 62, .show_sec = true}, 
    { .hour_page = 2, .hour_col = 38, .min_page = 2, .min_col = 55, .date_page = 0, .date_col = 1, .colon1_page = 2, .colon1_col = 50, .colon2_page = 1, .colon2_col = 53, .show_sec = false, .bmp = bmp_warning, .bmp_col = 70, .bmp_page = 2, .bmp_w = 12, .bmp_h = 12},
    { .hour_page = 2, .hour_col = 20, .min_page = 2,  .min_col = 44, .sec_page = 2, .sec_col = 68, .date_page = 0, .date_col = 14, .colon1_page = 2, .colon1_col = 38, .colon2_page = 2, .colon2_col = 62, .show_sec = true, .bmp = cartheme, .bmp_col = 0, .bmp_page = 0, .bmp_w = 128, .bmp_h = 64 }

};

uint8_t ismenu = 0;
bool devmode = 1;

// helper to convert uint into string
void u8_to_str(uint8_t val, char *buf)
{
    buf[0] = '0' + val / 10;
    buf[1] = '0' + val % 10;
    buf[2] = '\0';
}

void u16_to_str(uint16_t val, char *buf)
{
    buf[0] = '0' + val / 1000;
    buf[1] = '0' + (val % 1000) / 100;
    buf[2] = '0' + (val % 100)  / 10;
    buf[3] = '0' + val % 10;
    buf[4] = '\0';
}

// led part

void LED_Init(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef g = {
        .GPIO_Pin   = LED_PIN,
        .GPIO_Mode  = GPIO_Mode_Out_PP,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(LED_PORT, &g);
}

void LED_Set(uint8_t on)
{
    GPIO_WriteBit(LED_PORT, LED_PIN, on ? Bit_SET : Bit_RESET);
}
void Blink(const Pattern *p)
{
    for (uint8_t i = 0; i < p->count; i++) {
        LED_Set(1); Delay_Ms(p->on);
        LED_Set(0); Delay_Ms(p->off);
    }
}

// ui part

static void SSD1306_HLine(uint8_t page, uint8_t pattern)
{
    SSD1306_Cmd(0xB0 + page);
    SSD1306_Cmd(0x00); SSD1306_Cmd(0x10);
    for (uint8_t col = 0; col < 128; col++)
        SSD1306_Data(pattern);
}


void Draw_Clock(RTC_Time *now, RTC_Time *prev)
{
    const UI_Theme *t = &themes[current_theme];
    char buf[12];

        if (themes[current_theme].bmp != NULL || !themedrawn) {
        themedrawn = true;
        SSD1306_DrawBitmap(themes[current_theme].bmp_page, themes[current_theme].bmp_col, themes[current_theme].bmp, themes[current_theme].bmp_w, themes[current_theme].bmp_h);
    }

    if (now->day != prev->day || now->month != prev->month || now->year != prev->year || force_refresh) {
        u8_to_str(now->day,   buf);     buf[2] = '/';
        u8_to_str(now->month, buf+3);   buf[5] = '/';
        u16_to_str(now->year, buf+6);   buf[10] = '\0';
        SSD1306_Print(t->date_page, t->date_col, buf);
        SSD1306_HLine(1, 0x08);
    }

    if (now->hour != prev->hour || force_refresh) {
        u8_to_str(now->hour, buf);
        SSD1306_Print(t->hour_page, t->hour_col, buf);
    }

    if (now->min != prev->min || force_refresh) {
        u8_to_str(now->min, buf);
        SSD1306_Print(t->min_page, t->min_col, buf);
    }

    if (t->show_sec) {
        u8_to_str(now->sec, buf);
        SSD1306_Print(t->sec_page, t->sec_col, buf);
    }

    if (force_refresh) {
        SSD1306_Print(t->colon1_page, t->colon1_col, ":");
        SSD1306_Print(t->colon2_page, t->colon2_col, ":");
    }

    *prev = *now;
}

void Draw_Inputs(void)
{
    // Clear row 3
    SSD1306_Cmd(0xB0 + 3);
    SSD1306_Cmd(0x00); SSD1306_Cmd(0x10);
    for (uint8_t i = 0; i < 128; i++) SSD1306_Data(0x00);

    if (Btn_Pressed(BTN_UP))  {
        Blink(&PAT_OK);
        SSD1306_Print(3,  0, "UP");
        ismenu = 1;
    }
    if (Btn_Pressed(BTN_DN)) {
    SSD1306_Print(3, 50, "DN");
    devmode = 0;
    SSD1306_Print(0, 80, "        ");
    } 

    if (Btn_Pressed(BTN_CLK)) {
        SSD1306_Print(3, 98, "CLK");
        devmode = 1;
    }
    if (devmode) SSD1306_Print(0, 80, "devmode");
}

