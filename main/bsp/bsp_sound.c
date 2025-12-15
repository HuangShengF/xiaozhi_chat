#include "bsp_sound.h"

#define SOUND_I2C_PORT I2C_NUM_0
#define I2C_SCL_PIN GPIO_NUM_1
#define I2C_SDA_PIN GPIO_NUM_0

#define I2S_MCK_PIN GPIO_NUM_3
#define I2S_BCK_PIN GPIO_NUM_2
#define I2S_DATA_WS_PIN GPIO_NUM_5
#define DATA_OUT_PIN GPIO_NUM_6
#define I2S_DATA_IN_PIN GPIO_NUM_4

#define PA_PIN GPIO_NUM_7

static i2c_master_bus_handle_t i2c_bus_handle;
static i2s_chan_handle_t i2c_tx_handle;
static i2s_chan_handle_t i2c_rx_handle;
static esp_codec_dev_handle_t codec_dev;
static void bsp_iic_init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {0};
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = SOUND_I2C_PORT;
    i2c_bus_config.scl_io_num = I2C_SCL_PIN;
    i2c_bus_config.sda_io_num = I2C_SDA_PIN;
    i2c_bus_config.glitch_ignore_cnt = 7;               // SCL时钟线和SDA数据线的滤波器宽度，单位为时钟周期，默认为7
    i2c_bus_config.flags.enable_internal_pullup = true; // 使能内部上拉
    i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
}

static void bsp_i2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    chan_cfg.auto_clear = true;


    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),                            // 16KHz
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO), // 16bit, 单声道
        .gpio_cfg = {
            .mclk = I2S_MCK_PIN,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_DATA_WS_PIN,
            .dout = DATA_OUT_PIN,
            .din = I2S_DATA_IN_PIN,
        },
        .clk_cfg.clk_src = I2S_CLK_SRC_DEFAULT // 使用内部时钟
    };

    // 创建I2S通道
    i2s_new_channel(&chan_cfg, &i2c_tx_handle, &i2c_rx_handle);

    // 初始化通道
    i2s_channel_init_std_mode(i2c_tx_handle, &std_cfg);
    i2s_channel_init_std_mode(i2c_rx_handle, &std_cfg);

    // 使能I2S
    i2s_channel_enable(i2c_tx_handle);
    i2s_channel_enable(i2c_rx_handle);
}

static void bsp_es8311_init(void)
{
    // I2S配置
    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = i2c_rx_handle,
        .tx_handle = i2c_tx_handle
    };
    //数据配置
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    // I2C配置
    audio_codec_i2c_cfg_t i2c_cfg = {.addr = ES8311_CODEC_DEFAULT_ADDR, // I2C地址  es8311的设备地址
                                     .bus_handle = i2c_bus_handle,  // I2C总线句柄
                                     .port = SOUND_I2C_PORT, // I2C总线端口
                                    };
    // I2C控制配置
    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    // GPIO配置
    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();


     // es8311 配置
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = PA_PIN, //使能引脚
        .use_mclk = true, // 使用mclk: 主时钟，，这里必须有时钟，要么给他外部单独开一个时钟，要么主机给芯片提供一个时钟
    };
    //  创建 编解码器接口
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    // 解码器的配置
    esp_codec_dev_cfg_t dev_cfg = {
	.codec_if = out_codec_if,             // codec interface from es8311_codec_new
	.data_if = data_if,                    // data interface from audio_codec_new_i2s_data
	.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT, // codec support both playback and record
    };

    // 创建设备
    codec_dev = esp_codec_dev_new(&dev_cfg);
    
    //采样的设置
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000, // 采样率16k，一秒钟播放16k个采样点
        .channel = 1, // 单通道
        .bits_per_sample = 16,  // 采样位数16bit，，相当于ADC的位数
    };
    
    // 开启设备
    esp_codec_dev_open(codec_dev, &fs);
    //设置音量
    esp_codec_dev_set_out_vol(codec_dev, 40.0); 
    // 设置增益（放大增益）
    esp_codec_dev_set_in_gain(codec_dev, 10.0);
}


Doorbell_state bsp_write_dat(uint8_t *data, uint16_t size)
{
    // 参数检查
    if (data == NULL || size == 0) {
        MY_LOGE("Invalid parameters for bsp_write_dat");
        return COM_UTILT_ERROR;
    }

    // 检查设备是否已初始化
    if (codec_dev == NULL) {
        MY_LOGE("Codec device not initialized");
        return COM_UTILT_ERROR;
    }
    // MY_LOGE("bsp_write_dat");
    // 写入数据
    int ret = esp_codec_dev_write(codec_dev, data, size);
    if (ESP_CODEC_DEV_OK != ret) {
        MY_LOGE("Failed to write data to codec device, error: %d", ret);
        return COM_UTILT_ERROR;
    }
    
    return COM_UTILT_OK;
}

Doorbell_state bsp_read_dat(uint8_t *data, uint16_t size)
{
    // 参数检查
    if (data == NULL || size == 0) {
        MY_LOGE("Invalid parameters for bsp_read_dat");
        return COM_UTILT_ERROR;
    }

    // 检查设备是否已初始化
    if (codec_dev == NULL) {
        MY_LOGE("Codec device not initialized");
        return COM_UTILT_ERROR;
    }

    // 读取数据
    int ret = esp_codec_dev_read(codec_dev, data, size);
    if (ESP_CODEC_DEV_OK != ret) {
        MY_LOGE("Failed to read data from codec device, error: %d", ret);
        return COM_UTILT_ERROR;
    }
    
    return COM_UTILT_OK;
}

void bsp_sound_init(void)
{
    MY_LOGE("bsp_sound_init");

    // 初始化IIC
    bsp_iic_init();
    // 初始化I2S
    bsp_i2s_init();
    // 初始化es8311,音频解码芯片
    bsp_es8311_init();
}


static uint8_t volume = 50;
void bsp_sound_mute(bool sta)
{ 
    if(sta)
    {
        esp_codec_dev_set_out_vol(codec_dev, 0);
    }
    else
    {
        esp_codec_dev_set_out_vol(codec_dev, 50); 
    }
}

void bsp_sound_set_volume(uint8_t vol)
{ 
    volume = vol;
    esp_codec_dev_set_out_vol(codec_dev, volume); 
}