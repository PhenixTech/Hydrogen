#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ch32v00X.h"
#include "button.h"
#include "i2c.h"
#include "drivers/pcf8563.h"
#include "drivers/ssd1306.h"
#include "menu.h"
#include "states.h"
#include "ui.h"
#include "utils.h"

enum { WIDTH = 128, HEIGHT = 64, SCALE = 6, LED_AREA = 72 };

uint32_t SystemCoreClock = 48000000;
void *GPIOD;
void *ADC1;

static SDL_Window *window;
static SDL_Renderer *renderer;
static uint8_t panel[WIDTH * HEIGHT / 8];
static uint8_t pressed[3];
static bool led_on;
static bool display_on = true;
static bool panel_dirty;
static uint8_t oled_page, oled_col;
static time_t rtc_epoch;
static uint32_t rtc_started;

static int key_index(SDL_Keycode key)
{
    if (key == SDLK_UP) return 0;
    if (key == SDLK_DOWN) return 1;
    if (key == SDLK_SPACE || key == SDLK_RETURN) return 2;
    return -1;
}

static void pump(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) exit(0);
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            if (event.key.keysym.sym == SDLK_ESCAPE) exit(0);
            int i = key_index(event.key.keysym.sym);
            if (i >= 0) pressed[i] = 1;
        }
    }
}

static void draw_panel(void)
{
    panel_dirty = false;
    SDL_SetRenderDrawColor(renderer, 5, 9, 12, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            if (display_on && (!legacy_mode || !(y & 1)) &&
                (panel[((legacy_mode ? y / 2 : y) / 8) * WIDTH + x] &
                 (1u << ((legacy_mode ? y / 2 : y) & 7)))) {
                SDL_Rect pixel = { x * SCALE, y * SCALE, SCALE - 1, SCALE - 1 };
                SDL_RenderFillRect(renderer, &pixel);
            }
    int cx = WIDTH * SCALE + LED_AREA / 2, cy = HEIGHT * SCALE / 2;
    SDL_SetRenderDrawColor(renderer, led_on ? 255 : 55, led_on ? 35 : 8, led_on ? 25 : 8, 255);
    for (int y = -14; y <= 14; ++y)
        for (int x = -14; x <= 14; ++x)
            if (x * x + y * y <= 14 * 14) SDL_RenderDrawPoint(renderer, cx + x, cy + y);
    SDL_RenderPresent(renderer);
}

static void refresh_if_dirty(void)
{
    if (panel_dirty) draw_panel();
}

void Delay_Ms(uint32_t ms)
{
    uint32_t until = SDL_GetTicks() + ms;
    do { pump(); refresh_if_dirty(); SDL_Delay(1); } while ((int32_t)(until - SDL_GetTicks()) > 0);
}

uint32_t millis(void) { return SDL_GetTicks(); }
void SysTick_Init(void) {}
void Enter_Standby(void) {}
void InitializeADC(void) {}
void EXTI_Wake_Init(void) {}
uint8_t checkLowBat(void) { return 0; }
void draw_splash(void)
{
    SSD1306_On();
    FB_Clear();
    legacy_mode = 0;
    SSD1306_Init();
    FB_DrawBitmap(0, 0, splash_screen, 128, 64);
    FB_Update();
    Delay_Ms(2000);
    FB_Clear();
    legacy_mode = 1;
    SSD1306_Init();
    FB_Update();
}
void VLflagWarning(void)
{
    FB_Clear();
    FB_DrawBitmap(0, 90, bmp_warning, 12, 12);
    FB_Print(0, 20, "WARNING");
    FB_Print(1, 20, "RTC RESET!");
    FB_Print(2, 20, "SET TIME");
    FB_Print(3, 20, "AND DATE");
    FB_Update();
    while (!Btn_Pressed(BTN_CLK) && !Btn_Pressed(BTN_UP) && !Btn_Pressed(BTN_DN)) Delay_Ms(1);
    FB_Clear();
    FB_Update();
    setclock();
}
void LowBattery(uint8_t level)
{
    if (level == 0) return;
    FB_Clear();
    FB_DrawBitmap(1, 90, bmp_warning, 12, 12);
    FB_Print(0, 20, "WARNING");
    FB_Print(1, 20, level == 2 ? "CRITICAL BATTERY" : "LOW BATTERY");
    FB_Print(2, 20, "DETECTED!");
    FB_Print(3, 20, "CHANGE BATTERY!");
    FB_Update();
    while (!Btn_Pressed(BTN_CLK) && !Btn_Pressed(BTN_UP) && !Btn_Pressed(BTN_DN)) Delay_Ms(1);
    FB_Clear();
    FB_Update();
}
void RCC_PB2PeriphClockCmd(int peripheral, int enabled) { (void)peripheral; (void)enabled; }
void GPIO_Init(void *port, GPIO_InitTypeDef *config) { (void)port; (void)config; }
void GPIO_WriteBit(void *port, uint16_t pin, int value)
{
    (void)port;
    if (pin == GPIO_Pin_2) { led_on = value == Bit_SET; draw_panel(); }
}
uint16_t ADC_GetConversionValue(void *adc) { (void)adc; return 1800; }

void Buttons_Init(void) {}
uint8_t Btn_Pressed(uint16_t pin)
{
    pump();
    refresh_if_dirty();
    int i = pin == BTN_UP ? 0 : pin == BTN_DN ? 1 : pin == BTN_CLK ? 2 : -1;
    if (i < 0 || !pressed[i]) return 0;
    pressed[i] = 0;
    return 1;
}
uint32_t Btn_HoldMs(uint16_t pin) { return Btn_Pressed(pin) ? HOLD_MS : 0; }

static uint8_t bcd(unsigned n) { return (uint8_t)(((n / 10) << 4) | (n % 10)); }
static unsigned dec(uint8_t n) { return (n >> 4) * 10 + (n & 15); }
static struct tm rtc_now(void)
{
    time_t value = rtc_epoch + (SDL_GetTicks() - rtc_started) / 1000;
    struct tm result = *localtime(&value);
    return result;
}

void I2C_Init_Bus(void) {}
void I2C_Stop_Bus(void) {}
uint8_t I2C_Probe(uint8_t addr) { return addr == 0x3c || addr == 0x51 ? 0 : 1; }
uint8_t I2C_WriteReg(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (addr == 0x3c) {
        if (reg == 0x00) {
            if ((value & 0xf8) == 0xb0) oled_page = value & 7;
            else if ((value & 0xf0) == 0x00) oled_col = (oled_col & 0xf0) | (value & 0x0f);
            else if ((value & 0xf0) == 0x10) oled_col = (oled_col & 0x0f) | ((value & 0x0f) << 4);
            else if (value == 0xae) { display_on = false; draw_panel(); }
            else if (value == 0xaf) { display_on = true; draw_panel(); }
        } else if (reg == 0x40 && oled_page < 8 && oled_col < WIDTH) {
            panel[oled_page * WIDTH + oled_col++] = value;
            panel_dirty = true;
        }
        return 0;
    }
    if (addr != 0x51 || reg < 2 || reg > 8) return 0;
    struct tm t = rtc_now();
    switch (reg) {
    case 2: t.tm_sec = dec(value & 0x7f); break;
    case 3: t.tm_min = dec(value & 0x7f); break;
    case 4: t.tm_hour = dec(value & 0x3f); break;
    case 5: t.tm_mday = dec(value & 0x3f); break;
    case 7: t.tm_mon = (int)dec(value & 0x1f) - 1; break;
    case 8: t.tm_year = (int)dec(value) + 100; break;
    }
    rtc_epoch = mktime(&t); rtc_started = SDL_GetTicks();
    return 0;
}
uint8_t I2C_ReadBurst(uint8_t addr, uint8_t reg, uint8_t *out, uint8_t len)
{
    if (addr != 0x51 || reg != 2 || len < 7) return 1;
    struct tm t = rtc_now();
    out[0] = bcd(t.tm_sec); out[1] = bcd(t.tm_min); out[2] = bcd(t.tm_hour);
    out[3] = bcd(t.tm_mday); out[4] = bcd(t.tm_wday);
    out[5] = bcd((unsigned)t.tm_mon + 1); out[6] = bcd((unsigned)(t.tm_year % 100));
    return 0;
}
uint8_t I2C_ReadReg(uint8_t addr, uint8_t reg, uint8_t *out)
{
    uint8_t values[7];
    if (addr == 0x51 && reg >= 2 && reg <= 8 && !I2C_ReadBurst(addr, 2, values, 7)) {
        *out = values[reg - 2]; return 0;
    }
    *out = 0; return 0;
}
uint8_t I2C_WriteBurst(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (addr == 0x3c && reg == 0x40 && len == sizeof panel) {
        memcpy(panel, data, sizeof panel); draw_panel();
    }
    return 0;
}

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1;
    }
    window = SDL_CreateWindow("HYDROGEN simulator — arrows + Space", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WIDTH * SCALE + LED_AREA, HEIGHT * SCALE, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (window && !renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!window || !renderer) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    rtc_epoch = time(NULL); rtc_started = SDL_GetTicks();
    Buttons_Init(); I2C_Init_Bus(); SSD1306_Init(); PCF8563_Init();
    RTC_Time now, previous = { .hour = 255 };
    force_refresh = true;
    puts("HYDROGEN simulator: Up/Down arrows, Space or Enter for click, Esc to quit");
    for (;;) {
        PCF8563_GetTime(&now);
        Draw_Clock(&now, &previous);
        Draw_Inputs();
        showMenu();
        Delay_Ms(50);
    }
}
