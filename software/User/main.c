#include "ch32v00X_it.h"
#include "debug.h"
#include "ssd1306.h"
#include "i2c.h"
#include "bitmap.h"
#include <stdbool.h>

// ── Pin config ───────────────────────────────────────────────────────────────
#define LED_PIN   GPIO_Pin_2
#define LED_PORT  GPIOD
#define BTN_UP    GPIO_Pin_5  // PD5
#define BTN_DN    GPIO_Pin_4  // PD4
#define BTN_CLK   GPIO_Pin_6  // PD6

#define ID 
#define PCF8563_ADDR  0x51

#define DISPLAY_ON_MS  3000  // display stays on 3s 
#define HOLD_MS         50   // min hold to wake display 

typedef struct { uint16_t on; uint16_t off; uint8_t count; } Pattern;
// static const Pattern PAT_NOT_FOUND = { 800, 800, 1 };
static const Pattern PAT_INIT_FAIL = { 100, 100, 2 };
static const Pattern PAT_OK        = {  80,  80, 1 };

bool VL = 0;
bool devmode = 1;

// ── LED ──────────────────────────────────────────────────────────────────────
static void LED_Init(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef g = {
        .GPIO_Pin   = LED_PIN,
        .GPIO_Mode  = GPIO_Mode_Out_PP,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(LED_PORT, &g);
}

static void LED_Set(uint8_t on)
{
    GPIO_WriteBit(LED_PORT, LED_PIN, on ? Bit_SET : Bit_RESET);
}

static void Blink(const Pattern *p)
{
    for (uint8_t i = 0; i < p->count; i++) {
        LED_Set(1); Delay_Ms(p->on);
        LED_Set(0); Delay_Ms(p->off);
    }
}

// ── BCD ──────────────────────────────────────────────────────────────────────
static uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

// ── PCF8563 ──────────────────────────────────────────────────────────────────
typedef struct {
    uint8_t sec, min, hour, day, month;
    uint16_t year;
} RTC_Time;

static uint8_t PCF8563_Init(void)
{
    if (I2C_Probe(PCF8563_ADDR) != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x00, 0x00) != 0) return 2;
    if (I2C_WriteReg(PCF8563_ADDR, 0x01, 0x00) != 0) return 2;
    uint8_t val = 0xFF;
    if (I2C_ReadReg(PCF8563_ADDR, 0x00, &val) != 0) return 3;
    if (val != 0x00) return 3;
    return 0;
}

static uint8_t PCF8563_SetTime(RTC_Time *t)
{
    if (I2C_WriteReg(PCF8563_ADDR, 0x02, dec2bcd(t->sec))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x03, dec2bcd(t->min))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x04, dec2bcd(t->hour))        != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x05, dec2bcd(t->day))         != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x07, dec2bcd(t->month))       != 0) return 1;
    if (I2C_WriteReg(PCF8563_ADDR, 0x08, dec2bcd(t->year - 2000)) != 0) return 1;
    return 0;
}

static uint8_t PCF8563_GetTime(RTC_Time *t)
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

// Draw a horizontal line across full width on a page
static void SSD1306_HLine(uint8_t page, uint8_t pattern)
{
    SSD1306_Cmd(0xB0 + page);
    SSD1306_Cmd(0x00); SSD1306_Cmd(0x10);
    for (uint8_t col = 0; col < 128; col++)
        SSD1306_Data(pattern);
}

// ── Format helpers ───────────────────────────────────────────────────────────
static void u8_to_str(uint8_t val, char *buf)
{
    buf[0] = '0' + val / 10;
    buf[1] = '0' + val % 10;
    buf[2] = '\0';
}

static void u16_to_str(uint16_t val, char *buf)
{
    buf[0] = '0' + val / 1000;
    buf[1] = '0' + (val % 1000) / 100;
    buf[2] = '0' + (val % 100)  / 10;
    buf[3] = '0' + val % 10;
    buf[4] = '\0';
}

// ── Buttons ──────────────────────────────────────────────────────────────────
static void Buttons_Init(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef g = {
        .GPIO_Pin   = BTN_UP | BTN_DN | BTN_CLK,
        .GPIO_Mode  = GPIO_Mode_IPU,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(GPIOD, &g);
}

static uint8_t Btn_Pressed(uint16_t pin)
{
    return !GPIO_ReadInputDataBit(GPIOD, pin);
}

// Returns hold duration in ms, 0 if not pressed or released too fast
static uint32_t Btn_HoldMs(uint16_t pin)
{
    if (!Btn_Pressed(pin)) return 0;
    uint32_t ms = 0;
    while (Btn_Pressed(pin)) {
        Delay_Ms(10);
        ms += 10;
        if (ms > 5000) break; // safety cap
    }
    return ms;
}

// ── EXTI wake on PD6 (click) ─────────────────────────────────────────────────
static void EXTI_Wake_Init(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO, ENABLE);

    // Map PD6 to EXTI line 6
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource6);

    EXTI_InitTypeDef e = {
        .EXTI_Line    = EXTI_Line6,
        .EXTI_Mode    = EXTI_Mode_Interrupt,
        .EXTI_Trigger = EXTI_Trigger_Falling, // pull-up, press = falling
        .EXTI_LineCmd = ENABLE,
    };
    EXTI_Init(&e);

    NVIC_InitTypeDef n = {
        .NVIC_IRQChannel                   = EXTI7_0_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 1,
        .NVIC_IRQChannelSubPriority        = 0,
        .NVIC_IRQChannelCmd                = ENABLE,
    };
    NVIC_Init(&n);
}

// ISR, just clear flag, MCU wakes automatically
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
        EXTI_ClearITPendingBit(EXTI_Line6);
}

// ── Deep sleep ───────────────────────────────────────────────────────────────
static void Enter_Standby(void)
{
    SSD1306_Off();
    LED_Set(0);

    RCC_PB1PeriphClockCmd(RCC_PB1Periph_PWR, ENABLE);

    // Standby mode, wakes from EXTI
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);


    // Execution resumes here after wake (standby on V006 is actually stop mode)
    // that resets via EXTI, if true standby resets, remove code below)
}

// theme system 

typedef struct {
    uint8_t date_page,  date_col;
    uint8_t hour_page,  hour_col;
    uint8_t min_page,   min_col;
    uint8_t sec_page,   sec_col;
    const uint8_t *icon;
    uint8_t icon_page,  icon_col;
    const uint8_t *colon1;
    uint8_t colon1_page, colon1_col;
    const uint8_t *colon2;
    uint8_t colon2_page, colon2_col;
    bool show_sec;
} UI_Theme;

static const UI_Theme themes[] = {
    { .hour_page = 2, .hour_col = 20, .min_page = 2,  .min_col = 44, .sec_page = 2, .sec_col = 68, .date_page = 0, .date_col = 14, .colon1_page = 2, .colon1_col = 38, .colon2_page = 2, .colon2_col = 62, .show_sec = true}, 
    { .hour_page = 2, .hour_col = 38, .min_page = 2, .min_col = 55, .sec_page = 2, .sec_col = 56, .date_page = 0, .date_col = 1, .colon1_page = 2, .colon1_col = 50, .colon2_page = 1, .colon2_col = 53, .show_sec = false, .icon = bmp_warning, .icon_col = 70, .icon_page = 2}
};

uint8_t current_theme = 0;

// ── UI draw ──────────────────────────────────────────────────────────────────
static void Draw_Clock(RTC_Time *now, RTC_Time *prev)
{
    const UI_Theme *t = &themes[current_theme];
    char buf[12];

    if (now->day != prev->day || now->month != prev->month || now->year != prev->year) {
        u8_to_str(now->day,   buf);     buf[2] = '/';
        u8_to_str(now->month, buf+3);   buf[5] = '/';
        u16_to_str(now->year, buf+6);   buf[10] = '\0';
        SSD1306_Print(t->date_page, t->date_col, buf);
        SSD1306_HLine(1, 0x08);
    }

    if (now->hour != prev->hour) {
        u8_to_str(now->hour, buf);
        SSD1306_Print(t->hour_page, t->hour_col, buf);
    }

    if (now->min != prev->min || prev->hour == 255) {
        u8_to_str(now->min, buf);
        SSD1306_Print(t->min_page, t->min_col, buf);
    }

    if (t->show_sec) {
        u8_to_str(now->sec, buf);
        SSD1306_Print(t->sec_page, t->sec_col, buf);
    }

    if (prev->hour == 255) {
        SSD1306_Print(t->colon1_page, t->colon1_col, ":");
        SSD1306_Print(t->colon2_page, t->colon2_col, ":");
    }

    if (t->icon != NULL) {
        SSD1306_DrawBitmap(t->icon_page, t->icon_col, t->icon, 12, 12);
    }

    *prev = *now;
}

uint8_t ismenu = 0;
static void Draw_Inputs(void)
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

/*
// menu things v2
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
char buf[12];
void drawMenu(void) 
{
    SSD1306_Clear();

    u16_to_str(SystemCoreClock / 1000000, buf);  // shows "8" or "48"
    SSD1306_Print(3, 30, buf);

    SSD1306_DrawBitmap(2, 0, bmp_warning, 12, 12);
    SSD1306_Print(0,40, "BLINK OK");
    SSD1306_Print(1,40, "RTC SET");
    SSD1306_Print(2,40, "LEGACY MODE");
}

// menu

uint8_t new_min = 0;
uint8_t new_hour = 12;
bool menu_rtc = 0;
uint8_t time_sel = 0;
uint8_t new_date = 1;
uint16_t new_year = 2026;
uint8_t new_month = 1;
uint8_t cursor = 0;
uint8_t cursor_prev = 0;

bool menu_date = 0;
void setclock(void) {
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


static void showMenu(void)
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

// ── Main ─────────────────────────────────────────────────────────────────────
int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    LED_Init();
    Buttons_Init();
    EXTI_Wake_Init();
    Delay_Ms(300);
    I2C_Init_Bus();
    SSD1306_Clear();

    uint8_t rtc_ok  = (PCF8563_Init() == 0);
    uint8_t oled_ok = (SSD1306_Init() == 0);

    // VL flag check
    uint8_t sec_reg = 0;
    if (I2C_ReadReg(PCF8563_ADDR, 0x02, &sec_reg) == 0 && (sec_reg & 0x80)) {
        uint8_t VL = 1;
    }

    // Seed time comment out after first flash
    /* if (rtc_ok) {
         RTC_Time set = { .sec=0, .min=37, .hour=18, .day=29, .month=4, .year=2026 };
         PCF8563_SetTime(&set);
     }
    */
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
            // Display is off  enter standby, wake on EXTI (PD6, button UP)
            Enter_Standby();

            // Woke up : measure hold duration
            uint32_t hold = Btn_HoldMs(BTN_CLK);
            if (hold >= HOLD_MS) {
                // Long press : turn display on
                SSD1306_On();
                SystemCoreClockUpdate();
                Delay_Init();
                prev.hour = 255; // force full redraw
                if (rtc_ok) {
                    PCF8563_GetTime(&now);
                    Draw_Clock(&now, &prev);
                }
                disp_on  = 1;
                on_since = tick;
                Blink(&PAT_OK);
            }
            // Short press stay off loop back
        }

    }
}
