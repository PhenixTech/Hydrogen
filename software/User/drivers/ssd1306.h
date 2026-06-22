#include <stdint.h>

#ifndef SSD1306_H
#define SSD1306_H

uint8_t SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_On(void);
void SSD1306_Off(void);
void SSD1306_Print(uint8_t page, uint8_t col, const char *str);
void SSD1306_DrawBitmap(uint8_t page, uint8_t col, const uint8_t *bmp, uint8_t width, uint8_t height);
void SSD1306_Screenshot(void);


uint8_t SSD1306_Cmd(uint8_t cmd);  
uint8_t SSD1306_Data(uint8_t dat); 
extern uint8_t legacy_mode;

#endif