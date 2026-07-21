#include "ch32v00X_it.h"
#include "debug.h"
#include "drivers/ssd1306.h"
#include "i2c.h"
#include "bitmap.h"
#include "drivers/pcf8563.h"
#include "ui.h"
#include "button.h"
#include "menu.h"
#include <stdbool.h>
#include "ch32v00X_adc.h"
#include "utils.h"

#define DISPLAY_ON_MS  3000  // display stays on 3s 

// MAIN
int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    LED_Init();
    InitializeADC();
    Buttons_Init();
    EXTI_Wake_Init();
    Delay_Ms(300);
    I2C_Init_Bus();
    SSD1306_Clear();

    uint8_t rtc_ok  = (PCF8563_Init() == 0);
    uint8_t oled_ok = (SSD1306_Init() == 0);

    if (!rtc_ok && oled_ok) {
        SSD1306_Clear();
        SSD1306_Print(1, 20, "RTC ERROR");
        Delay_Ms(2000);
    }

    RTC_Time now;
    RTC_Time prev    = { .hour = 255 }; // sentinel: forces full first draw
    uint32_t tick    = 0;
    uint32_t on_since = 0;
    uint8_t  disp_on  = 0;

    // Cold boot show display immediately without requiring hold
    if (oled_ok) {
        Delay_Ms(500);
        SSD1306_Clear();
        force_refresh = true;
        SSD1306_On();
        if (rtc_ok) {
            PCF8563_GetTime(&now);
            Draw_Clock(&now, &prev);
        }
        disp_on  = 1;
        on_since = 0;
        Blink(&PAT_OK);
    }

    if (!oled_ok) {
    Blink(&PAT_INIT_FAIL);
    }

    while (1) {
        tick += 50;
        Delay_Ms(50);

        if (disp_on) {
            I2C_Init_Bus();
            InitializeADC();
            // Update display
            if (rtc_ok && oled_ok && PCF8563_GetTime(&now) == 0) {
                Draw_Clock(&now, &prev);
                Draw_Inputs();
                showMenu();
            }
            // Auto-off after DISPLAY_ON_MS
            if (!devmode && tick - on_since >= DISPLAY_ON_MS) {
                disp_on = 0;
                SSD1306_Off();
                LED_Set(0);
                ismenu = 0;
            }
        } else {
            // Display is off enter standby, wake on EXTI (PD6, button CLK)
            Enter_Standby();
            SystemCoreClockUpdate();
            Delay_Init();

            // Woke up : measure hold duration
            uint32_t hold = Btn_HoldMs(BTN_CLK);
            if (hold >= HOLD_MS) {
                // Long press : turn display on
                I2C_Init_Bus();
                InitializeADC();
                SSD1306_On();
                force_refresh = 1;
                if (rtc_ok) {
                    PCF8563_GetTime(&now);
                    Draw_Clock(&now, &prev);
                }
                checkVL();
                if (checkLowBat()) LowBattery();
                if (VL == 1) VLflagWarning();
                disp_on  = 1;
                on_since = tick;
                Blink(&PAT_OK);
            }
            // Short press stay off loop back
        }

    }
}