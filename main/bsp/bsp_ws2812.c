#include "bsp_ws2812.h"

#define LED_STRIP_GPIO_PIN  GPIO_NUM_46
#define LED_STRIP_LED_COUNT 2

#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

// Set to 1 to use DMA for driving the LED strip, 0 otherwise
// Please note the RMT DMA feature is only available on chips e.g. ESP32-S3/P4
#define LED_STRIP_USE_DMA  0


led_strip_handle_t led_strip;

void Bsp_W2812_Init(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };


    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };


    // 创建句柄
    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

void Bsp_W2812_Set_Color_ON(uint8_t *color)
{
    led_strip_set_pixel(led_strip, 0, color[0], color[1], color[2]);
    led_strip_set_pixel(led_strip, 1, color[0], color[1], color[2]);
    led_strip_refresh(led_strip);
}

void Bsp_W2812_OFF(void)
{
    led_strip_clear(led_strip);
}