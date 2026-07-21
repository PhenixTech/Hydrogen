#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#define BTN_UP    GPIO_Pin_5  // PD5
#define BTN_DN    GPIO_Pin_4  // PD4
#define BTN_CLK   GPIO_Pin_6  // PD6
#define HOLD_MS         400  // min hold to wake display 

void Buttons_Init(void);
uint8_t Btn_Pressed(uint16_t pin);
uint32_t Btn_HoldMs(uint16_t pin);

#endif