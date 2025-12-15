#ifndef __DISPLAY_H
#define __DISPLAY_H


#include "Com_Debug.h"
#include "Com_utilt.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "font_emoji.h"

/* LCD size */
#define LCD_H_RES (240)
#define LCD_V_RES (320)

/* LCD settings */
#define LCD_SPI_NUM (SPI2_HOST)
#define LCD_PIXEL_CLK_HZ (40 * 1000 * 1000)
#define LCD_CMD_BITS (8)          // 命令位数
#define LCD_PARAM_BITS (8)        // 给命令准备参数的位数
#define LCD_BITS_PER_PIXEL (16)   // 每个像素用16位  RGB565
#define LCD_DRAW_BUFF_DOUBLE (0)
#define LCD_DRAW_BUFF_HEIGHT (5)
#define LCD_BL_ON_LEVEL (1)   // 背光开关电平: 1-打开, 0-关闭

/* LCD pins */
#define LCD_GPIO_SCLK (GPIO_NUM_47)
#define LCD_GPIO_MOSI (GPIO_NUM_48)
#define LCD_GPIO_RST (GPIO_NUM_16)
#define LCD_GPIO_DC (GPIO_NUM_45)
#define LCD_GPIO_CS (GPIO_NUM_21)
#define LCD_GPIO_BL (GPIO_NUM_40)

void display_init(void);
void display_show_usage(void);
void display_show_tts_stt(char *stt_tts);
void display_show_llm(char *emotion);
#endif 
