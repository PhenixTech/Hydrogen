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
bool menu_rtc = 0;
bool force_refresh = 0;
uint8_t scroll = 0;
uint8_t time_sel = 0;
uint8_t page = 1;

uint8_t new_min = 0;
uint8_t new_hour = 12;
uint8_t new_date = 1;
uint16_t new_year = 2026;
uint8_t new_month = 1;

int8_t cursor = 0;
int8_t cursor_prev = 0;

char buf[12];

static void setclock(void) 
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

static void stopwatch(void)
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

static void MenuPage(uint8_t page)
{
    SSD1306_Clear();
    u8_to_str(page, buf);
    SSD1306_Print(1, 0, buf);
    switch(page) {
        case 1:
            u16_to_str(SystemCoreClock / 1000000, buf); 
            SSD1306_Print(3, 30, buf);
            uint16_t adcv = ADC_GetConversionValue(ADC1);
            adcv = (uint32_t)(1200 * 4095) / adcv;
            u16_to_str(adcv, buf);
            SSD1306_Print(3,90, buf);
            SSD1306_DrawBitmap(2, 0, bmp_warning, 12, 12);
            SSD1306_Print(0,40, "BLINK OK");
            SSD1306_Print(1,40, "RTC SET");
            SSD1306_Print(2,40, "LEGACY MODE");            
            break;
        case 2:
            SSD1306_Print(0, 40, "EXIT MENU");
            SSD1306_Print(1, 40, "STOPWATCH");
            SSD1306_Print(2, 40, "VL Flag");
            break;
        case 3:
            SSD1306_Print(0, 40, "Low Battery");
            SSD1306_Print(1, 40, "Splash Screen");
            break;            
        case 4:
            SSD1306_Print(0,40, "Firmware");
            SSD1306_Print(1,40, "and Hardware");
            SSD1306_Print(2,40, "By PhenixTech");    
    }
}

#define pages_amount 4
static void MoveCursor(int8_t dir)
{
    cursor_prev = cursor;
    cursor += dir ; 
        if (cursor >= pages_amount - 1) { 
            cursor = 0; // in legacy mode, there is 4 pages, normal mode get 8.
            if (page == 0 || page > pages_amount - 1) page = 0;
            page++ ;
            MenuPage(page); 
        } 
        if (cursor <= -1) {
            cursor = 0;
            page-- ;
            if (page > pages_amount - 1) page = 1;
            if (page == 0) ismenu = 0;
            MenuPage(page);
        }
    SSD1306_Print(cursor_prev, 20, " ");
    Delay_Ms(300);

}

void VLflagWarning()
{
    SSD1306_Clear();
    SSD1306_DrawBitmap(0,70, bmp_warning, 12,12);
    SSD1306_Print(0,20, "WARNING");
    SSD1306_Print(1,20, "VL Flag");
    SSD1306_Print(2,20, "has been set!");    
    SSD1306_Print(3,20, "Time Reset!");
    while (!Btn_Pressed(BTN_CLK) && !Btn_Pressed(BTN_UP) && !Btn_Pressed(BTN_DN));
    SSD1306_Clear();
}

void LowBattery(uint8_t level)
{
    if (level == 0) return;

    SSD1306_Clear();
    SSD1306_DrawBitmap(1,90, bmp_warning, 12,12);
    SSD1306_Print(0,20, "WARNING");
    SSD1306_Print(1,20, level == 2 ? "Critical battery" : "Low Battery");
    SSD1306_Print(2,20, "detected!");    
    SSD1306_Print(3,20, "Change battery!");

    while (!Btn_Pressed(BTN_CLK) && !Btn_Pressed(BTN_UP) && !Btn_Pressed(BTN_DN));
    SSD1306_Clear();
}


void showMenu(void)
{
    if(ismenu == 0) return;
    MenuPage(1);
    page = 1;
    while(ismenu == 1) {
    SSD1306_Print(cursor, 20, "@"); // could change it but i like the design on this one
        if (Btn_Pressed(BTN_UP)) {
            MoveCursor(-1);
        }
        if (Btn_Pressed(BTN_DN)) {
            MoveCursor(+1);
        }
        if (Btn_Pressed(BTN_CLK)) {
            switch(page) {
                case 1 :
                    switch(cursor) {
                        case 0 : { Blink(&PAT_OK); break; } //BLINK OK
                        case 1 : { SSD1306_Clear(); setclock(); ismenu = 0; break; } // SET RTC
                        case 2 : { legacy_mode = !legacy_mode; SSD1306_Init(); MenuPage(1); break; } // LEGACY MODE
                    }
                    break;
                case 2 :
                    switch(cursor) {
                        case 0 : { SSD1306_Clear(); ismenu = 0; break; }
                        case 1 : { SSD1306_Clear(); stopwatch(); ismenu = 0; break; }
                        case 2 : { VLflagWarning(); ismenu = 0; break;}
                    }
                    break;
                case 3 :
                    switch (cursor) {
                        case 0 : { LowBattery(1); LowBattery(2); ismenu = 0; break;}
                        case 1 : { draw_splash(); ismenu = 0; break;}
                    }
            }
        }
    }
}

