#include "ch32v00X.h"
#include "drivers/ssd1306.h"
#include "ui.h"
#include "button.h"
#include "menu.h"
#include <stdint.h>
#include <stdbool.h>
#include "drivers/ssd1306.h"

bool menu_rtc = 0;

uint8_t time_sel = 0;

uint8_t new_min = 0;
uint8_t new_hour = 12;
uint8_t new_date = 1;
uint16_t new_year = 2026;
uint8_t new_month = 1;

uint8_t cursor = 0;
uint8_t cursor_prev = 0;

char buf[12];
static void drawMenu(void) 
{
    SSD1306_Clear();

    u16_to_str(SystemCoreClock / 1000000, buf);  // shows "8" or "48"
    SSD1306_Print(3, 30, buf);

    SSD1306_DrawBitmap(2, 0, bmp_warning, 12, 12);
    SSD1306_Print(0,40, "BLINK OK");
    SSD1306_Print(1,40, "RTC SET");
    SSD1306_Print(2,40, "LEGACY MODE");
}


static void setclock(void) {
menu_rtc = 1;
    SSD1306_Clear();
    while(menu_rtc) {
    // .date_page = 0 .date_col = 14
        if (new_hour > 23) new_hour = 0;
        if (new_min > 59) new_min = 0;
        if (new_date > 31) new_date = 1;
        if (new_month > 12) new_month = 1;
        u8_to_str(new_min, buf);
        SSD1306_Print(2, 44, buf);

        SSD1306_Print(2, 38, ":");

        u8_to_str(new_hour, buf);
        SSD1306_Print(2, 20, buf);

        u8_to_str(new_date, buf);
        SSD1306_Print(0, 20, buf);

        u8_to_str(new_month, buf);
        SSD1306_Print(0, 40, buf);

        u16_to_str(new_year, buf);
        SSD1306_Print(0, 60, buf);

        if(Btn_Pressed(BTN_CLK)) {
            time_sel = time_sel + 1;
            if(time_sel >= 5) {
                RTC_Time set = { .sec=0, .min=new_min, .hour=new_hour, .day=new_date, .month=new_month, .year=new_year };
                PCF8563_SetTime(&set);
                time_sel = 0;
                SSD1306_Clear();
                SSD1306_Print(1, 50, "time set !");
                Delay_Ms(500);
                SSD1306_Clear(); 
                menu_rtc = 0;
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
                        new_hour = new_hour - 1;
                        Delay_Ms(150);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 1:
                        new_min = new_min - 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 44, "  ");
                        break;
                    case 2: 
                        new_date = new_date - 1;
                        Delay_Ms(50);
                        SSD1306_Print(2, 20, "  ");
                        break;
                    case 3 :
                        new_month = new_month - 1;
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


void showMenu(void)
{
    if(ismenu == 0) return;
    drawMenu();
    while(ismenu == 1) {
    SSD1306_Print(cursor, 20, "@"); // could change it but i like the design on this one
        if (Btn_Pressed(BTN_UP)) {
               cursor_prev = cursor;
               cursor = cursor + 1;
               if (cursor >= 3) cursor = 0; // in legacy mode, there is 4 pages, normal mode get 8.
               SSD1306_Print(cursor_prev, 20, " ");
               Delay_Ms(500);
        }
        if (Btn_Pressed(BTN_DN)) {
            ismenu = 0;
            SSD1306_Clear();
            return;
        }
        if (Btn_Pressed(BTN_CLK)) {
            switch(cursor) {
                case 0 : { //BLINK OK
                    Blink(&PAT_OK);
                    break;
                }
                case 1 : { // RTC SET
                    SSD1306_Clear();
                    setclock();
                    ismenu = 0;
                    break;
                }
                case 2 : { // LEGACY MODE
                    legacy_mode = !legacy_mode;
                    SSD1306_Init();
                    drawMenu();
                    break;
                }
            }

        }
    }
}








/* menu things v2
typedef struct {
    uint8_t count;
    const char **options;
} UI_menu;

static const char *main_menu_options[] = {
    "BLINK OK",
    "SET RTC",
    "LEGACY MODE"
};

static const UI_menu menu[] = {
    { .options = main_menu_options, .count = 3 }
};

uint8_t current_menu = 0;


void drawMenuV2(const UI_menu *m) {
    SSD1306_Clear();
    for (uint8_t i = 0; i < menu[current_menu].count; i++) {
    SSD1306_Print(i, 40, menu[current_menu].options[i]);
    }
}
*/
