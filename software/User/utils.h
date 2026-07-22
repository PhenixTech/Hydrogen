#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void Enter_Standby(void);
void InitializeADC();
void EXTI_Wake_Init(void);
uint8_t checkLowBat();
void draw_splash();

void SysTick_Init(void);
uint32_t millis(void);

#endif