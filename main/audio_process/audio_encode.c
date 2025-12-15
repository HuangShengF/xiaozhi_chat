#include "audio_encode.h"

audio_encode_t my_encode;

static esp_audio_enc_handle_t encoder = NULL;

void audio_encode_init(void)
{
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = 16000,
        .bits_per_sample = 16,                               // 16 位样本深度（PCM）。
        .bitrate = 32000,                                    // 目标码率设置为 32 kbps。
        .channel = 1,                                        // 单声道
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS, // 编码器处理的音频块大小对应 60 毫秒时长。这是 OPUS 编码的关键参数，它决定了输入帧的样本数。
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,
        .enable_fec = false, // 向前纠错
        .enable_vbr = false, // 可变码率
        .enable_dtx = false};

    esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &encoder);
}

void audio_encode_task(void *argv);
void audio_encode_start(RingbufHandle_t in_buf, RingbufHandle_t out_buf)
{
    my_encode.is_running = true;
    my_encode.in_buf = in_buf;
    my_encode.out_buf = out_buf;

    // xTaskCreate(audio_encode_task, "audio_encode_task", 16 * 1024, NULL, 5, NULL);
    xTaskCreatePinnedToCoreWithCaps(audio_encode_task, "audio_encode_task", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}

// 数据从in_data拷贝到out_data
void copy_buffer(RingbufHandle_t in_data, uint8_t *out_data, int in_size)
{
    // memcpy(out_data, in_data, out_size);
    
    while(in_size > 0)
    {
        size_t real_size = 0;
        //这个函数有坑，如果读数据读到末尾，我们指定的数据没读完成他不会从头开始，我们需要读取两次
        void *data = xRingbufferReceiveUpTo(in_data, &real_size, portMAX_DELAY, in_size); 
        memcpy(out_data, data, real_size);
        vRingbufferReturnItem(in_data, data);
        in_size -= real_size;
        out_data += real_size; //偏移数
    }
}

void audio_encode_task(void *argv)
{
    // Get needed buffer size and prepare memory
    int in_size = 0, out_size = 0;
    esp_opus_enc_get_frame_size(encoder, &in_size, &out_size);
    uint8_t *in_data = heap_caps_malloc(in_size, MALLOC_CAP_SPIRAM); // 现在是一帧一帧的转，只有一帧的缓冲区
    uint8_t *out_data = heap_caps_malloc(out_size, MALLOC_CAP_SPIRAM);

    esp_audio_enc_in_frame_t in_frame = {
        .buffer = in_data, // 输入数据（此时是为空的，需要从别的缓冲区拷贝过来）
        .len = in_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = out_data, // 转换后的数据
        .len = out_size,    // 输出数据的长度
    };

    // printf("in_size: %d, out_size: %d\n", in_size, out_size);   
    while (my_encode.is_running)
    {
        copy_buffer(my_encode.in_buf, in_data, in_size); // 从缓冲区中拷贝数据到in_data,编码器再从in_data拿到数据进行编码
        esp_opus_enc_process(encoder, &in_frame, &out_frame);
        xRingbufferSend(my_encode.out_buf, out_frame.buffer, out_frame.encoded_bytes, 0); //把编码后的数据放到缓冲区中,如果已经满了就丢弃这一帧
        
        // printf("%d %ld\n", out_size, out_frame.encoded_bytes);
    }

    heap_caps_free(in_data);
    heap_caps_free(out_data);
    vTaskDelete(NULL);
}