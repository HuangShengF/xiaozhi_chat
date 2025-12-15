#ifndef BSP_WS2812_H
#define BSP_WS2812_H
 

#include "Com_utilt.h"
#include "Com_Debug.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"


void Bsp_W2812_Init(void);

void Bsp_W2812_Set_Color_ON(uint8_t *color);

void Bsp_W2812_OFF(void);

#endif // BSP_WS2812_H


