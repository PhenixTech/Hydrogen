#include "ch32v00X.h"
#include "drivers/ssd1306.h"
#include "drivers/pcf8563.h"
#include "ui.h"
#include "button.h"
#include "menu.h"
#include <stdint.h>
#include <stdbool.h>
#include "drivers/ssd1306.h"
#include "utils.h"
#include "states.h"

uint8_t new_min = 0;
uint8_t new_hour = 12;
uint8_t new_date = 1;
uint16_t new_year = 2026;
uint8_t new_month = 1;
bool menu_cal = 0;

uint8_t time_sel = 0;

static const char *dow_labels[7]   = {"Su","Mo","Tu","We","Th","Fr","Sa"};
static const char *month_labels[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                                        "Jul","Aug","Sep","Oct","Nov","Dec"};
static char buf[12];


bool menu_rtc = 0;

void setclock(void) 
{
    menu_rtc = 1;
    PCF8563_ClearVL();
    VL = 0;
    SSD1306_Clear();
    SSD1306_Print(3, 20, "@@");
    while(menu_rtc) {
    // .date_page = 0 .date_col = 14
        if (new_hour > 23) new_hour = 0;
        if (new_min > 59) new_min = 0;
        if (new_date > 31) new_date = 1;
        if (new_month > 12) new_month = 1;

        u8_to_str(new_min, buf);
        SSD1306_Print(2, 44, buf);

        u8_to_str(new_hour, buf);
        SSD1306_Print(2, 20, buf);

        u8_to_str(new_date, buf);
        SSD1306_Print(0, 20, buf);

        u8_to_str(new_month, buf);
        SSD1306_Print(0, 40, buf);

        u16_to_str(new_year, buf);
        SSD1306_Print(0, 60, buf);
        SSD1306_Print(2, 38, ":");
        SSD1306_Print(0, 32, "/");
        SSD1306_Print(0, 54, "/");

        if(Btn_Pressed(BTN_CLK)) {
            time_sel = time_sel + 1;
            switch(time_sel) 
            {
                case 1:
                SSD1306_Print(3, 20, "  ");
                SSD1306_Print(3, 44, "@@");
                break;
                case 2:
                SSD1306_Print(3, 44, "  ");
                SSD1306_Print(1, 20, "@@");
                break;
                case 3:
                SSD1306_Print(1, 20, "  ");
                SSD1306_Print(1, 40, "@@");
                break;
                case 4:
                SSD1306_Print(1, 40, "  ");
                SSD1306_Print(1, 60, "@@@@");
                break;
            }
            if(time_sel >= 5) {
                RTC_Time set = { .sec=0, .min=new_min, .hour=new_hour, .day=new_date, .month=new_month, .year=new_year };
                PCF8563_SetTime(&set);
                time_sel = 0;
                SSD1306_Clear();
                SSD1306_Print(1, 50, "time set !");
                Delay_Ms(500);
                SSD1306_Clear(); 
                menu_rtc = 0;
                force_refresh = 1;
                }
            Delay_Ms(500);
        }

        if (Btn_Pressed(BTN_UP)) 
        { 
            switch(time_sel)
                {
                    case 0:
                        new_hour = new_hour + 1;
                        Delay_Ms(150);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 1:
                        new_min = new_min + 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 44, "  ");
                        break;
                    case 2: 
                        new_date = new_date + 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 3 :
                        new_month = new_month + 1;
                        Delay_Ms(150);
                        SSD1306_Print(2, 44, "  ");
                        break;
                    case 4 :
                        new_year = new_year + 1;
                        Delay_Ms(200);
                        SSD1306_Print(2, 60, "    ");
                        break;
                    }

        }

                if (Btn_Pressed(BTN_DN)) 
        { 
            switch(time_sel)
                {
                    case 0:
                        new_hour = (new_hour == 0) ? 23 : new_hour - 1;
                        Delay_Ms(150);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 1:
                        new_min = (new_min == 0) ? 59 : new_min - 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 44, "  ");
                        break;
                    case 2: 
                        new_date = (new_date <= 1) ? 31 : new_date - 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 3 :
                        new_month = (new_month <= 1) ? 12 : new_month - 1;
                        Delay_Ms(150);
                        SSD1306_Print(2, 44, "  ");
                        break;
                    case 4 :
                        new_year = new_year - 1;
                        Delay_Ms(200);
                        SSD1306_Print(2, 60, "    ");
                        break;
                }
            }
        }
}

bool sw_started = false;
bool menu_sw = 0;

void stopwatch(void)
{
    menu_sw = 1;
    while(menu_sw)  {
        uint32_t sw_start_ms = 0;
        uint32_t sw_elapsed  = 0;
        sw_start_ms = millis();
        sw_elapsed = millis() - sw_start_ms;
        uint8_t sec  = (sw_elapsed / 1000) % 60;
        uint8_t mins = (sw_elapsed / 60000) % 60;
        u8_to_str(mins, buf); SSD1306_Print(0, 20, buf);
        SSD1306_Print(0, 32, ":");
        u8_to_str(sec, buf);  SSD1306_Print(0, 38, buf);
    if (Btn_Pressed(BTN_UP)) { if (sw_started == 0) {SSD1306_Print(2,20, "cleared"); Delay_Ms(200); SSD1306_Print(2,20, "       "); Delay_Ms(300);} } 
    if (Btn_Pressed(BTN_DN)) { if (sw_started == 1) {SSD1306_Print(1,20, "new lap"); Delay_Ms(200); SSD1306_Print(1,20, "       "); Delay_Ms(300); } }
    if (Btn_Pressed(BTN_CLK)) { if (sw_started == 0) { SSD1306_Print(0,20, "Watch started"); sw_started = 1; Delay_Ms(300);}
                                else { sw_started = 0; SSD1306_Print(0, 20,"              "); } Delay_Ms(300);}
    if (Btn_HoldMs(BTN_UP)) { if (sw_started == 0) { menu_sw = 0;} }
    }
}

static uint8_t DayOfWeek(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}
 
static uint8_t DaysInMonth(uint16_t y, uint8_t m)
{
    static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return dim[m-1];
}
 
static void Draw_Calendar(uint8_t view_month, uint16_t view_year, RTC_Time *today)
{
    SSD1306_Clear();
 
    char hdr[10];
    uint8_t i = 0;
    const char *mn = month_labels[view_month - 1];
    while (mn[i]) { hdr[i] = mn[i]; i++; }
    hdr[i++] = ' ';
    char ybuf[6];
    u16_to_str(view_year, ybuf);
    for (uint8_t j = 0; ybuf[j]; j++) hdr[i++] = ybuf[j];
    hdr[i] = '\0';
    SSD1306_Print(0, 34, hdr);
 
    for (uint8_t c = 0; c < 7; c++)
        SSD1306_Print(1, 2 + c * 18, dow_labels[c]);
 
    uint8_t first_dow = DayOfWeek(view_year, view_month, 1);
    uint8_t days       = DaysInMonth(view_year, view_month);
    bool this_month    = (view_month == today->month && view_year == today->year);
 
    for (uint8_t d = 1; d <= days; d++) {
        uint8_t cell  = first_dow + (d - 1);
        uint8_t row   = cell / 7;
        uint8_t col_i = cell % 7;
        uint8_t p     = 2 + row;
        uint8_t col   = 2 + col_i * 18;
 
        u8_to_str(d, buf);
        if (this_month && d == today->day)
            SSD1306_PrintBoxed(p, col, 16, buf);
        else
            SSD1306_Print(p, col, buf);
    }
}
 
void calendar(void)
{
    RTC_Time today;
    PCF8563_GetTime(&today);
 
    uint8_t  view_month = today.month;
    uint16_t view_year  = today.year;
 
    // grid needs all 8 pages, force normal mode for the duration
    bool was_legacy = legacy_mode;
    if (legacy_mode) { legacy_mode = 0; SSD1306_Init(); }
 
    menu_cal = 1;
    Draw_Calendar(view_month, view_year, &today);
 
    while (menu_cal) {
        if (Btn_Pressed(BTN_DN)) {
            if (++view_month > 12) { view_month = 1; view_year++; }
            Draw_Calendar(view_month, view_year, &today);
            Delay_Ms(250);
        }
        if (Btn_Pressed(BTN_UP)) {
            if (view_month == 1) { view_month = 12; view_year--; }
            else view_month--;
            Draw_Calendar(view_month, view_year, &today);
            Delay_Ms(250);
        }
        if (Btn_Pressed(BTN_CLK)) {
            menu_cal = 0;
            Delay_Ms(200);
        }
    }
 
    if (was_legacy) { legacy_mode = 1; SSD1306_Init(); }
    SSD1306_Clear();
}
