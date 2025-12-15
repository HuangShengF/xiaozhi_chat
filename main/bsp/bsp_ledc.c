
#include "bsp_ledc.h"

// 背光控制配置
#define BACKLIGHT_PIN     40      // 背光控制引脚
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_FREQ         50000    // PWM频率(Hz)，建议1k-20k
#define LEDC_RESOLUTION   LEDC_TIMER_8_BIT  // PWM分辨率

static const char *TAG = "BACKLIGHT";

/**
 * @brief 初始化背光PWM控制器
 */
void backlight_init(void)
{
    // 1. 配置PWM定时器
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_RESOLUTION,
        .freq_hz          = LEDC_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. 配置PWM通道
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BACKLIGHT_PIN,
        .duty           = 0,           // 初始占空比设为0
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // 3. 设置初始亮度为50%
    backlight_set_brightness(90);
    
    ESP_LOGI(TAG, "Backlight PWM initialized");
}

/**
 * @brief 设置背光亮度
 * @param brightness 亮度值 (0-255对应0%-100%)
 */
void backlight_set_brightness(uint8_t brightness)
{
    // 设置PWM占空比
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}