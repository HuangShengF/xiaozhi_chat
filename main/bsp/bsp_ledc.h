#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include "driver/ledc.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化背光控制
void backlight_init(void);

// 设置背光亮度 (0-255)
void backlight_set_brightness(uint8_t brightness);


#ifdef __cplusplus
}
#endif

#endif // BACKLIGHT_H