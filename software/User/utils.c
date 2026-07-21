#include "ch32v00X.h"
#include "drivers/ssd1306.h"
#include "i2c.h"
#include "ui.h"
#include "utils.h"

// EXTI (interupt) wake on PD6 (click)
void EXTI_Wake_Init(void)
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

void InitializeADC()
{
     ADC_InitTypeDef ADC_InitStructure = {0};
     RCC_PB2PeriphClockCmd(RCC_PB2Periph_ADC1, ENABLE);
     RCC_ADCCLKConfig(RCC_PCLK2_Div8);

     ADC_DeInit(ADC1);
     ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
     ADC_InitStructure.ADC_ScanConvMode = DISABLE;
     ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; 
     ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; 
     ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
     ADC_InitStructure.ADC_NbrOfChannel = 1; 
     ADC_Init(ADC1, &ADC_InitStructure);

     ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_CyclesMode7);

     ADC_Cmd(ADC1, ENABLE);

     ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

// Deep sleep 
void Enter_Standby(void)
{
    SSD1306_Off();
    LED_Set(0);
    I2C_Stop_Bus();

    RCC_PB1PeriphClockCmd(RCC_PB1Periph_PWR, ENABLE);

    ADC_Cmd(ADC1, DISABLE);
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_ADC1, DISABLE);

    // Standby mode, wakes from EXTI
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);


    // Execution resumes here after wake (standby on V006 is actually stop mode disable swio and stuff)
}


volatile uint32_t sys_ms = 0;

void SysTick_Init(void) {
    SysTick->CMP  = SystemCoreClock / 1000 - 1;
    SysTick->CNT  = 0;
    SysTick->SR   = 0;
    SysTick->CTLR = (1<<0)|(1<<1)|(1<<2)|(1<<3);
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void SysTick_Handler(void) {
    SysTick->SR &= ~1;
    sys_ms++;
}

uint32_t millis(void) { return sys_ms; }

bool checkLowBat() {
    uint16_t adcv = ADC_GetConversionValue(ADC1);
    adcv = (uint32_t)(1200 * 4095) / adcv;
    if (adcv < 2500) {
        return true;
    }
    else return false;
}