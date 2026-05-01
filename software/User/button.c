#include "ch32v00X.h"
#include "button.h"
#include <stdint.h>

void Buttons_Init(void)
{
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef g = {
        .GPIO_Pin   = BTN_UP | BTN_DN | BTN_CLK,
        .GPIO_Mode  = GPIO_Mode_IPU,
        .GPIO_Speed = GPIO_Speed_30MHz,
    };
    GPIO_Init(GPIOD, &g);
}

uint8_t Btn_Pressed(uint16_t pin)
{
    return !GPIO_ReadInputDataBit(GPIOD, pin);
}

// Returns hold duration in ms, 0 if not pressed or released too fast
uint32_t Btn_HoldMs(uint16_t pin)
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