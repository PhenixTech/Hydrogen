#include "ch32v00X.h"
#include "drivers/ssd1306.h"
#include "ui.h"
#include "button.h"
#include "menu.h"
#include <stdint.h>
#include <stdbool.h>
#include "drivers/ssd1306.h"
#include "utils.h"
#include "states.h"

bool force_refresh = 0;
uint8_t scroll = 0;

uint8_t page = 1;

int8_t cursor = 0;
int8_t cursor_prev = 0;

const char *fw_build = __DATE__ " " __TIME__;

static char buf[12];

static void MenuPage(uint8_t page)
{
    FB_Clear();
    u8_to_str(page, buf);
    FB_Print(1, 0, buf);
    switch(page) {
        case 1:
            u16_to_str(SystemCoreClock / 1000000, buf); 
            FB_Print(3, 30, buf);
            uint16_t adcv = ADC_GetConversionValue(ADC1);
            adcv = (uint32_t)(1200 * 4095) / adcv;
            u16_to_str(adcv, buf);
            FB_Print(3,90, buf);
            FB_DrawBitmap(2, 0, bmp_warning, 12, 12);
            FB_Print(0,40, "BLINK OK");
            FB_Print(1,40, "RTC SET");
            FB_Print(2,40, "LEGACY MODE");
            break;
        case 2:
            FB_Print(0, 40, "EXIT MENU");
            FB_Print(1, 40, "STOPWATCH");
            FB_Print(2, 40, "VL Flag");
            break;
        case 3:
            FB_Print(0, 40, "Low Battery");
            FB_Print(1, 40, "Splash Screen");
            FB_Print(2, 40, "Calendar");
            break;            
        case 4:
            FB_Print(0,40, "Firmware");
            FB_Print(1,40, "and Hardware");
            FB_Print(2,40, "By PhenixTech");    
            FB_Print(3,0, fw_build);
            break;    
        default:
            FB_Clear();
            ismenu = 0;
            break;
    }
    FB_Update();
}

#define pages_amount 4

static void MoveCursor(int8_t dir)
{
    cursor_prev = cursor;
    cursor += dir ; 
        if (cursor >= 3) { 
            cursor = 0; // in legacy mode, there is 4 pages (SSD1306 pages!), normal mode get 8. 
            if (page == 0 || page > pages_amount - 1) page = 0;
            page++ ;
            MenuPage(page); 
        } 
        if (cursor <= -1) {
            cursor = 0;
            page-- ;
            if (page > pages_amount - 1) page = 1;
            MenuPage(page);
        }
    FB_Print(cursor_prev, 20, " ");
    FB_Update();

}

void showMenu(void)
{
    if(ismenu == 0) return;
    MenuPage(1);
    page = 1;
    while(ismenu == 1) {
    FB_Print(cursor, 20, "@"); // could change it but i like the design on this one
    FB_Update();
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
                        case 2 : { calendar(); ismenu = 0; break;}
                    }
                    break;
                case 4 :
                    switch (cursor) {
                        case 0 : { bright_menu(); ismenu = 0; break;}
                    }
            }
        }
    }
}

