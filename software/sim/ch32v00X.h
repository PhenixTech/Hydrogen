#ifndef HYDROGEN_SIM_CH32V00X_H
#define HYDROGEN_SIM_CH32V00X_H

#include <stdint.h>
#include <stddef.h>

#define GPIO_Pin_2 (1u << 2)
#define GPIO_Pin_4 (1u << 4)
#define GPIO_Pin_5 (1u << 5)
#define GPIO_Pin_6 (1u << 6)
#define GPIO_Mode_Out_PP 0
#define GPIO_Speed_30MHz 0
#define RCC_PB2Periph_GPIOD 0
#define ENABLE 1
#define DISABLE 0
#define Bit_SET 1
#define Bit_RESET 0

typedef struct { uint16_t GPIO_Pin; int GPIO_Mode; int GPIO_Speed; } GPIO_InitTypeDef;

extern uint32_t SystemCoreClock;
extern void *GPIOD;
extern void *ADC1;

void RCC_PB2PeriphClockCmd(int peripheral, int enabled);
void GPIO_Init(void *port, GPIO_InitTypeDef *config);
void GPIO_WriteBit(void *port, uint16_t pin, int value);
uint16_t ADC_GetConversionValue(void *adc);
void Delay_Ms(uint32_t ms);

#endif
