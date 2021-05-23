#include "lcd_lib.h"
#include "font.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "dma2d.h"
#include "stdarg.h"
#include "string.h"
#include "usart.h"

uint8_t __LCD_Layer = 0;
uint8_t __LCD_Dir = 0; // 1 横屏 0 竖屏

#define LCD_DBG_SIZE 12
#define LCD_DBG_COLOR LCD_RGB(90, 200, 80)
#define LCD_DBG_COLOR_BG LCD_RGB(100, 50, 150) // 背景颜色

//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
uint16_t LCD_BGR2RGB(uint16_t c)
{
  uint16_t  r, g, b, rgb;
  b = (c >> 0) & 0x1f;
  g = (c >> 5) & 0x3f;
  r = (c >> 11) & 0x1f;
  rgb = (b << 11) + (g << 5) + (r << 0);
  return(rgb);
}

HAL_StatusTypeDef LCD_SetAlpha(uint8_t alpha, uint8_t alpha0, uint8_t layer)
{
  __HAL_LOCK(&hltdc);
  LTDC_LAYER(&hltdc, layer)->CACR &= ~(LTDC_LxCACR_CONSTA);
  LTDC_LAYER(&hltdc, layer)->CACR = alpha;
  hltdc.Instance->SRCR = LTDC_SRCR_IMR;
  hltdc.State = HAL_LTDC_STATE_READY;
  __HAL_UNLOCK(&hltdc);
	return HAL_OK;
}

void LCD_SwitchLayer(uint8_t layer)
{
  __LCD_Layer = layer;
}

void LCD_setDir(uint8_t dir)
{
  __LCD_Dir = dir;
}

uint32_t LCD_RGB(uint8_t r, uint8_t g, uint8_t b)
{
  r = r > 255 ? 255 : r;
  g = g > 255 ? 255 : g;
  b = b > 255 ? 255 : b;
  return (r << 16) | (g << 8) | b;
}

uint16_t LCD_888to556(uint32_t color)
{

  uint32_t r = ((0x00FF0000 & color) >> 8) & 0x0000F800;
  uint32_t g = ((0x0000FF00 & color) >> 5) & 0x000007E0;
  uint32_t b = (0x000000FF & color) >> 3;
  return r | g | b;
}

uint32_t LCD_565to888(uint16_t color)
{
  uint32_t r = color & 0x0000F800;
  uint32_t g = color & 0x000007E0;
  uint32_t b = color & 0x0000001F;
  return (r << 8) | (g << 5) | (b << 3);
}

//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color)
{
  if (__LCD_Dir == LCD_DIR_H) // 横屏
    __LCDBuff[__LCD_Layer][y * LCD_WIDTH + x] = LCD_888to556(color);
  else
    __LCDBuff[__LCD_Layer][(LCD_HIGHT - x - 1) * LCD_WIDTH + y] = LCD_888to556(color);
  return;
}

//读点函数
//返回值:颜色值
uint32_t LCD_Read_Point(uint16_t x, uint16_t y)
{
  return LCD_565to888(__LCDBuff[__LCD_Layer][y * LCD_WIDTH + x]);
}

//pwm:背光等级,0~100.越大越亮.
void LCD_BackLightSet(uint8_t pwm)
{
  TIM3->CCR2 = (int)(pwm * 10 / 1000);
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(uint32_t color)
{
  LCD_Fill(0, 0, LCD_WIDTH, LCD_HIGHT, color);
}
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
  uint32_t addr = (uint32_t)__LCDBuff[__LCD_Layer] + sizeof(uint16_t) * (sy * LCD_WIDTH + sx);
  hdma2d.Init.Mode = DMA2D_R2M;
  hdma2d.Init.OutputOffset = LCD_WIDTH - (ex - sx);
  HAL_DMA2D_Init(&hdma2d);
  HAL_DMA2D_Start(&hdma2d, color, addr, ex - sx, ey - sy);
  HAL_DMA2D_PollForTransfer(&hdma2d, 0xff);
}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
  uint16_t t;
  int xerr = 0, yerr = 0, delta_x, delta_y, distance;
  int incx, incy, uRow, uCol;
  delta_x = x2 - x1; //计算坐标增量 
  delta_y = y2 - y1;
  uRow = x1;
  uCol = y1;
  if (delta_x > 0)incx = 1; //设置单步方向 
  else if (delta_x == 0)incx = 0;//垂直线 
  else { incx = -1; delta_x = -delta_x; }
  if (delta_y > 0)incy = 1;
  else if (delta_y == 0)incy = 0;//水平线 
  else { incy = -1; delta_y = -delta_y; }
  if (delta_x > delta_y)distance = delta_x; //选取基本增量坐标轴 
  else distance = delta_y;
  for (t = 0; t <= distance + 1; t++)//画线输出 
  {
    LCD_DrawPoint(uRow, uCol, color);//画点 
    xerr += delta_x;
    yerr += delta_y;
    if (xerr > distance)
    {
      xerr -= distance;
      uRow += incx;
    }
    if (yerr > distance)
    {
      yerr -= distance;
      uCol += incy;
    }
  }
}
//画矩形    
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
  LCD_DrawLine(x1, y1, x2, y1, color);
  LCD_DrawLine(x1, y1, x1, y2, color);
  LCD_DrawLine(x1, y2, x2, y2, color);
  LCD_DrawLine(x2, y1, x2, y2, color);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint32_t color)
{
  int a, b;
  int di;
  a = 0; b = r;
  di = 3 - (r << 1);             //判断下个点位置的标志
  while (a <= b)
  {
    LCD_DrawPoint(x0 + a, y0 - b, color);             //5
    LCD_DrawPoint(x0 + b, y0 - a, color);             //0           
    LCD_DrawPoint(x0 + b, y0 + a, color);             //4               
    LCD_DrawPoint(x0 + a, y0 + b, color);             //6 
    LCD_DrawPoint(x0 - a, y0 + b, color);             //1       
    LCD_DrawPoint(x0 - b, y0 + a, color);
    LCD_DrawPoint(x0 - a, y0 - b, color);             //2             
    LCD_DrawPoint(x0 - b, y0 - a, color);             //7                
    a++;
    //使用Bresenham算法画圆     
    if (di < 0)di += 4 * a + 6;
    else
    {
      di += 10 + 4 * (a - b);
      b--;
    }
  }
}
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24/32
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint32_t color)
{
  uint8_t temp, t1, t;
  uint16_t y0 = y;
  uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);    //得到字体一个字符对应点阵集所占的字节数  
  uint16_t width = __LCD_Dir == LCD_DIR_H ? LCD_WIDTH : LCD_HIGHT;
  uint16_t hight = __LCD_Dir == LCD_DIR_H ? LCD_HIGHT : LCD_WIDTH;
  num = num - ' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
  for (t = 0; t < csize; t++)
  {
    if (size == 12)temp = asc2_1206[num][t];      //调用1206字体
    else if (size == 16)temp = asc2_1608[num][t];  //调用1608字体
    else if (size == 24)temp = asc2_2412[num][t];  //调用2412字体
    else if (size == 32)temp = asc2_3216[num][t];  //调用3216字体
    else return;                //没有的字库
    for (t1 = 0; t1 < 8; t1++)
    {
      if (temp & 0x80) 
      {
        LCD_DrawPoint(x, y, color);
      }
      temp <<= 1;
      y++;
      if (y > hight)
        return;    //超区域了
      if ((y - y0) == size)
      {
        y = y0;
        x++;
        if (x > width)
          return;  //超区域了
        break;
      }
    }
  }
}

//显示数字,高位为0,则不显示
//x,y :起点坐标   
//len :数字的位数
//size:字体大小
//color:颜色 
//num:数值(0~4294967295);   
void LCD_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color)
{
  uint8_t t, temp;
  uint8_t enshow = 0;
  for (t = 0; t < len; t++)
  {
    temp = (num / (uint32_t)pow(10, len - t - 1)) % 10;
    if (enshow == 0 && t < (len - 1))
    {
      if (temp == 0)
      {
        LCD_ShowChar(x + (size / 2) * t, y, ' ', size, color);
        continue;
      }
      else enshow = 1;

    }
    LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, color);
  }
}
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);   
//len:长度(即要显示的位数)
//size:字体大小
//mode:0,不填充;1,填充0.
void LCD_ShowxNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint32_t color)
{
  uint8_t t, temp;
  uint8_t enshow = 0;
  for (t = 0; t < len; t++)
  {
    temp = (num / (uint32_t)pow(10, len - t - 1)) % 10;
    if (enshow == 0 && t < (len - 1))
    {
      if (temp == 0)
      {
        LCD_ShowChar(x + (size / 2) * t, y, ' ', size, color);
        continue;
      }
      else enshow = 1;

    }
    LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, color);
  }
}
//显示字符串
//x,y:起点坐标
//width,height:区域大小  
//size:字体大小
//*p:字符串起始地址      
void LCD_ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, uint8_t* p, uint32_t color)
{
  uint8_t x0 = x;
  if (__LCD_Dir == LCD_DIR_H)
  {
    if (x > LCD_WIDTH || y > LCD_HIGHT)
    {
      return;
    }
    if (width == 0 || width >= LCD_WIDTH - x)
      width = LCD_WIDTH - x;
    if (height == 0 || LCD_HIGHT - y)
      height = LCD_HIGHT - y;
  }
  else
  {
    if (x > LCD_HIGHT || y > LCD_WIDTH)
    {
      return;
    }
    if (width == 0 || width >= LCD_HIGHT - x)
      width = LCD_HIGHT - x;
    if (height == 0 || LCD_WIDTH - y)
      height = LCD_WIDTH - y;
  }
  width += x;
  height += y;
  while ((*p <= '~') && (*p >= ' '))//判断是不是非法字符!
  {
    if (x > width - size / 2)
    {
      x = x0;
      y += size;
    }
    if (y > height)break;//退出
    LCD_ShowChar(x, y, *p, size, color);
    x += size / 2;
    p++;
  }
}

void LCD_RefreshFromMemory(uint16_t* src, uint16_t* dst)
{
  // uint32_t addr = (uint32_t)__LCDBuff[__LCD_Layer] + sizeof(uint16_t) * (sy * LCD_WIDTH + sx);
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.OutputOffset = 0;
  HAL_DMA2D_Init(&hdma2d);
  HAL_DMA2D_Start(&hdma2d, (uint32_t)src, (uint32_t)dst, LCD_WIDTH, LCD_HIGHT);
  HAL_DMA2D_PollForTransfer(&hdma2d, 0xff);
}

void LCD_Print(uint16_t x, uint16_t y, uint8_t size, uint32_t color, uint8_t* buf, ...)
{
  uint8_t str[200] = { 0 };
  va_list v;
  va_start(v, buf);
  vsprintf((char*)str, (char*)buf, v); //使用可变参数的字符串打印。类似sprintf
  LCD_ShowString(x, y, 0, 0, size, str, color);
  va_end(v);
}

void LCD_Debug(uint8_t* buf, ...)
{
  static uint8_t str[LCD_WIDTH / (LCD_DBG_SIZE / 2) * LCD_HIGHT / LCD_DBG_SIZE] = { 0 }; // 最大容纳字符和横竖屏无关  竖屏是 LCD_WIDTH / 12 * LCD_HIGHT / (12 / 2)
  static uint8_t show_line = 0;
  uint8_t str_tmp[200] = { 0 };
  uint16_t x_limit = __LCD_Dir == LCD_DIR_H ? LCD_WIDTH / (LCD_DBG_SIZE / 2) : LCD_HIGHT / (LCD_DBG_SIZE / 2);
  uint16_t y_limit = __LCD_Dir == LCD_DIR_H ? LCD_HIGHT / LCD_DBG_SIZE : LCD_WIDTH / LCD_DBG_SIZE;
  uint16_t y_new;
  uint16_t str_len;
  va_list v;
  va_start(v, buf);
  vsprintf((char*)str_tmp, (char*)buf, v); // 使用可变参数的字符串打印。类似sprintf
  va_end(v);
  LCD_SwitchLayer(!__LCD_Layer); // 0 1 层切换显示
  LCD_Clear(LCD_DBG_COLOR_BG);
  str_len = strlen((char*)str_tmp);
  y_new = str_len % x_limit ? str_len / x_limit + 1 : str_len / x_limit; // 此处进一取整
  if (show_line + y_new >= y_limit)
  {
    uint16_t y_src;
    if (show_line < y_limit)
    {
      y_src = show_line + y_new - y_limit;
      show_line = y_limit;
    }
    else
    {
      y_src = y_new;
    }
    for (uint16_t i = 0; i < (y_limit - y_src) * x_limit; i++)
    {
      if (str[i] != str[i + y_src * x_limit])
      {
        str[i] = str[i + y_src * x_limit];
      }
    }
    memset(&(str[(y_limit - y_new) * x_limit]), 0, y_new * x_limit);
    memcpy(&(str[(y_limit - y_new) * x_limit]), str_tmp, str_len);
  }
  else
  {
    memcpy(&(str[show_line * x_limit]), str_tmp, str_len);
    show_line += y_new;
  }
  for (uint16_t i = 0; i < y_limit; i++)
  {
    if (i == 0 || str[i * x_limit - 1] == 0) // 前一行末尾为0就显示  否则跟着前一行打印
    {
      LCD_ShowString(0, i * LCD_DBG_SIZE, 0, 0, LCD_DBG_SIZE, &(str[i * x_limit]), LCD_DBG_COLOR);
    }
  }
  LCD_SetAlpha(0, 0, !__LCD_Layer);
  LCD_SetAlpha(255, 0, __LCD_Layer);
}


void LCD_ColorTest(uint32_t delay)
{
  // 0 up 1 down
  uint8_t r_ctrl = 0;
  uint8_t g_ctrl = 0;
  uint8_t b_ctrl = 0;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t r_cnt = 255;
  uint8_t g_cnt = 255;
  uint8_t b_cnt = 255;
  while (1)
  {
    r_cnt++;
    g_cnt++;
    b_cnt++;
    // r
    if (r_cnt == 255)
    {
      r_cnt = 0;
      r_ctrl = rand() % 2;
    }
    if (r_ctrl == 0 && r < 255)
    {
      r++;
    }
    else if (r_ctrl == 1 && r > 0)
    {
      r--;
    }
    // g
    if (g_cnt == 255)
    {
      g_cnt = 0;
      g_ctrl = rand() % 2;
    }
    if (g_ctrl == 0 && g < 255)
    {
      g++;
    }
    else if (g_ctrl == 1 && g > 0)
    {
      g--;
    }
    // b
    if (b_cnt == 255)
    {
      b_cnt = 0;
      b_ctrl = rand() % 2;
    }
    if (b_ctrl == 0 && b < 255)
    {
      b++;
    }
    else if (b_ctrl == 1 && b > 0)
    {
      b--;
    }
    LCD_Clear(LCD_RGB(r, g, b));
    HAL_Delay(delay);
  }
}
