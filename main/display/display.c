#include "display.h"

#include "lv_examples.h"

static const char *TAG = "display";

/* LCD IO and panel */
static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;

/* LVGL display and touch */
static lv_display_t *lvgl_disp = NULL;

// 中文字体
LV_FONT_DECLARE(font_puhui_14_1);
// LV_FONT_DECLARE(font_puhui_16_4);

static void display_lcd_init(void)
{
    // esp_err_t ret = ESP_OK;

    /* LCD backlight  背光引脚配置*/
    gpio_config_t bk_gpio_config = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_GPIO_BL};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    // lcd 配置
    const spi_bus_config_t buscfg = {
        .sclk_io_num     = LCD_GPIO_SCLK,
        .mosi_io_num     = LCD_GPIO_MOSI,
        .miso_io_num     = GPIO_NUM_NC,
        .quadwp_io_num   = GPIO_NUM_NC,
        .quadhd_io_num   = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),   // 内部缓冲区的大小
    };
    spi_bus_initialize(LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO);

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num       = LCD_GPIO_DC,
        .cs_gpio_num       = LCD_GPIO_CS,
        .pclk_hz           = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits      = LCD_CMD_BITS,
        .lcd_param_bits    = LCD_PARAM_BITS,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, &io_config, &lcd_io);

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_GPIO_RST,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,   // 颜色顺序
        .bits_per_pixel = LCD_BITS_PER_PIXEL,
    };
    esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel);

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);   // 打开屏幕

    /* LCD backlight on */
    gpio_set_level(LCD_GPIO_BL, LCD_BL_ON_LEVEL);
}
static void display_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority     = 4,    /* LVGL task priority */
        .task_stack        = 1024 * 8, /* LVGL task stack size */
        .task_affinity     = -1,   /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500,  /* Maximum sleep in LVGL task */
        .timer_period_ms   = 5     /* LVGL timer tick period in ms */
    };
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle     = lcd_io,
        .panel_handle  = lcd_panel,
        .buffer_size   = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = LCD_DRAW_BUFF_DOUBLE,
        .hres          = LCD_H_RES,
        .vres          = LCD_V_RES,
        .monochrome    = false,   // 是否单色(只有黑白)显示
        .color_format  = LV_COLOR_FORMAT_RGB565,
        .rotation      = {
                 .swap_xy  = false,
                 .mirror_x = true,
                 .mirror_y = true,
        },
        .flags = {
            .buff_dma    = false,   // 是否使用DMA
            .swap_bytes  = true,   // 字节序调整
            .buff_spiram = false,   // 是否使用SPIRAM
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
}

lv_obj_t *tts_stt_lable;
lv_obj_t *llm_lable;
void display_default_show(void)
{
    lv_obj_t *scr = lv_scr_act();

    lvgl_port_lock(0);

    lv_obj_t *my_lable = lv_label_create(scr);
    lv_obj_set_width(my_lable, LCD_H_RES);
    lv_obj_set_style_text_align(my_lable, LV_TEXT_ALIGN_CENTER, 0);   //  文本在label中的对齐方式
    lv_label_set_text(my_lable, "鳄鱼不饿");
    lv_obj_align(my_lable, LV_ALIGN_CENTER, 0, -140);
    lv_obj_set_style_text_font(my_lable, &font_puhui_14_1, 0);   // 设置使用中文字体

    tts_stt_lable = lv_label_create(scr);
    lv_obj_set_width(tts_stt_lable, LCD_H_RES);
    lv_obj_set_style_text_align(tts_stt_lable, LV_TEXT_ALIGN_CENTER, 0);   //  文本在label中的对齐方式
    lv_label_set_text(tts_stt_lable, "你好世界");
    lv_obj_align(tts_stt_lable, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_text_font(tts_stt_lable, &font_puhui_14_1, 0);   // 设置使用中文字体

    llm_lable = lv_label_create(scr);
    lv_obj_set_width(llm_lable, LCD_H_RES);
    lv_obj_set_style_text_align(llm_lable, LV_TEXT_ALIGN_CENTER, 0);   //  文本在label中的对齐方式
    lv_label_set_text(llm_lable, "😂");
    lv_obj_align(llm_lable, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(llm_lable, font_emoji_64_init(), 0);   // 设置使用中文字体

    lvgl_port_unlock();
}

void display_init(void)
{
    display_lcd_init();

    display_lvgl_init();

    display_default_show();
}


void display_show_usage(void)
{
    lvgl_port_lock(0);
    lv_label_set_text(tts_stt_lable, "说:\"你好,小智,唤醒设备\"");
    lvgl_port_unlock();
}

void display_show_tts_stt(char *stt_tts)
{
    lvgl_port_lock(0);
    lv_label_set_text(tts_stt_lable, stt_tts);
    lvgl_port_unlock();
}

char *emoji_mapping[42] = {
    "neutral",
    "😶",
    "happy",
    "🙂",
    "laughing",
    "😆",
    "funny",
    "😂",
    "sad",
    "😔",
    "angry",
    "😠",
    "crying",
    "😭",
    "loving",
    "😍",
    "embarrassed",
    "😳",
    "surprised",
    "😯",
    "shocked",
    "😱",
    "thinking",
    "🤔",
    "winking",
    "😉",
    "cool",
    "😎",
    "relaxed",
    "😌",
    "delicious",
    "🤤",
    "kissy",
    "😘",
    "confident",
    "😏",
    "sleepy",
    "😴",
    "silly",
    "😜",
    "confused",
    "🙄",
};
void display_show_llm(char *emotion)
{
    char *text = "😍";
    for(int i = 0; i < 42; i += 2)
    {
        if(strcmp(emotion, emoji_mapping[i]) == 0)
        {
            text = emoji_mapping[i + 1];
            break;
        }
    }

    lvgl_port_lock(0);
    lv_label_set_text(llm_lable, text);
    lvgl_port_unlock();
}
