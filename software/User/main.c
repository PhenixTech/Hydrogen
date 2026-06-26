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


#define DISPLAY_ON_MS  3000  // display stays on 3s 

// EXTI (interupt) wake on PD6 (click)
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
static void Enter_Standby(void)
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
            // Display is off enter standby, wake on EXTI (PD6, button UP)
            Enter_Standby();

            // Woke up : measure hold duration
            uint32_t hold = Btn_HoldMs(BTN_CLK);
            if (hold >= HOLD_MS) {
                // Long press : turn display on
                I2C_Init_Bus();
                SSD1306_On();
                SystemCoreClockUpdate();
                Delay_Init();
                force_refresh = 1;
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