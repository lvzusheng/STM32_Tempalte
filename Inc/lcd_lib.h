#ifndef __LCD_LIB_H__
#define __LCD_LIB_H__		
#include "stdlib.h"
#include "stm32h7xx.h"
#include "ltdc.h"

//扫描方向定义
#define LCD_DIR_H  1 		//横屏
#define LCD_DIR_V  0 		//竖屏

void LCD_ColorTest(uint32_t delay);
uint32_t LCD_RGB(uint8_t r, uint8_t g, uint8_t b);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);
uint32_t LCD_Read_Point(uint16_t x, uint16_t y);
HAL_StatusTypeDef LCD_SetAlpha(uint8_t alpha, uint8_t alpha0, uint8_t layer);
void LCD_BackLightSet(uint8_t pwm);
void LCD_Clear(uint32_t color);
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint32_t color);
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint32_t color);
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color);
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color);
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t* p, uint32_t color); \
void LCD_RefreshFromMemory(uint16_t* src, uint16_t* dst);
void LCD_Print(uint16_t x, uint16_t y, uint8_t size, uint32_t color, uint8_t* buf, ...);
void LCD_setDir(uint8_t dir);
void LCD_Debug(uint8_t* buf, ...);
#endif  
