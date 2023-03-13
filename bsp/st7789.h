#ifndef __ST7789_H
#define __ST7789_H

/* 颜色 */
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40
#define BRRED 			 0XFC07
#define GRAY  			 0X8430

extern uint16_t POINT_COLOR;	/* 画笔颜色 */
extern uint16_t BACK_COLOR;  	/* 背景色 */ 

void screen_init();
void lcd_clear(uint16_t color);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_show_char(uint16_t x, uint16_t y,char num,uint8_t size,uint8_t mode);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height,char *p,uint8_t size,uint8_t mode);
void lcd_show_chinese(uint16_t x, uint16_t y, const uint8_t * ch,uint8_t size,uint8_t n,uint8_t mode);

void lcd_task_init();

#endif