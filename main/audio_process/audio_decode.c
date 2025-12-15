#include "audio_decode.h"

audio_decode_t my_decoder;
static esp_audio_dec_handle_t decoder;
void audio_decode_init(void)
{

    // Register AAC decoder, or you can call `esp_audio_dec_register_default` to register all supported decoder
    esp_opus_dec_register();

    // Configuration for AAC decoder
    esp_opus_dec_cfg_t opus_cfg = {
        .sample_rate = 16000,
        .channel = 1,
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,
        .self_delimited = false,
    };
    // memset(&write_ctx, 0, sizeof(write_ctx));
    // write_ctx.use_common_api = true;
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg),
    };

    esp_audio_dec_open(&dec_cfg, &decoder);
}

void audio_decode_task(void *arg);
void audio_decode_start(RingbufHandle_t in_buf)
{
    my_decoder.is_running = true;
    my_decoder.in_buf = in_buf;

    // 创建任务
    xTaskCreatePinnedToCoreWithCaps(audio_decode_task, "audio_decode_task", 32 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
}


void audio_decode_task(void *arg)
{
    // 分配解码输出缓冲区（一次分配，重复使用）
    uint8_t *out_buffer = heap_caps_malloc(32 * 1024, MALLOC_CAP_SPIRAM);
    if (!out_buffer) {
        MY_LOGE("Failed to allocate output buffer");
        vTaskDelete(NULL);
        return;
    }

    esp_audio_dec_out_frame_t out_frame = {
        .buffer = out_buffer,
        .len = 32 * 1024  // 注意：这个 len 是 buffer 容量，不是有效数据长度
    };

    while (my_decoder.is_running)
    {
        size_t item_size = 0;
        uint8_t *input_data = (uint8_t *)xRingbufferReceive(my_decoder.in_buf, &item_size, portMAX_DELAY);
        if (!input_data) {
            continue; // 理论上不会发生，因为用了 portMAX_DELAY
        }

        esp_audio_dec_in_raw_t raw = {
            .buffer = input_data,
            .len = item_size
        };

        uint8_t *original_ptr = input_data; // 用于最后 vRingbufferReturnItem

        // 解码整个输入块，可能需要多次调用（取决于编解码器）
        while (raw.len > 0 && my_decoder.is_running)
        {
            // 这里解码器不断去读取raw.buffer，直到读取完毕或者解码器认为输入数据已经处理完毕,也要不断更新buf
            esp_err_t ret = esp_audio_dec_process(decoder, &raw, &out_frame);
            if (ret != ESP_OK) {
                MY_LOGE("Decode error: %d", ret);
                break;
            }

            // consumed 表示本次解码消耗了多少输入字节
            // 消耗的是opus格式的字节，而解码的是PCM格式的字节，PCM比较大没压缩，而OPUS是压缩的，所以消耗的字节比输出的字节要小很多
            // raw.consumed < out_frame.decoded_size
            // MY_LOGE("Consumed: %ld, Output: %ld bytes", raw.consumed, out_frame.decoded_size);

            // 更新 raw 缓冲区指针和长度，因为每次解码器都读取raw缓冲区的数据,如果不更新会反复读取同一数据
            raw.buffer += raw.consumed;
            raw.len   -= raw.consumed;

            // 如果有有效输出数据，则播放
            bsp_write_dat(out_frame.buffer, out_frame.decoded_size);   
        }
        // 归还 ring buffer 数据
        vRingbufferReturnItem(my_decoder.in_buf, original_ptr);
    }

    // 清理资源
    heap_caps_free(out_buffer);
    vTaskDelete(NULL);
}

// void audio_decode_task(void *arg)
// {
//     // 从websocket发来的数据放到的缓冲区
//     esp_audio_dec_in_raw_t raw = {};
//     // esp_audio_dec_out_frame_t *out_frame = &write_ctx.out_frame;
//     // int ret = 0;
//     esp_audio_dec_out_frame_t out_frame = {
//         .buffer = heap_caps_malloc(32 * 1024, MALLOC_CAP_SPIRAM),
//         .len = 32 * 1024};

//     while (my_decoder.is_running)
//     {
//         // 从缓冲区拿数据，并进行解码
//         // esp_audio_dec_out_frame_t *out_frame = NULL;
//         // 一次只解码一帧
//         raw.buffer = (uint8_t *)xRingbufferReceive(my_decoder.in_buf, &raw.len, portMAX_DELAY);
//         void *storge_ptr = raw.buffer;
//         while (raw.len > 0)
//         {
//             MY_LOGE("raw.len = %d", raw.len);
//             esp_audio_dec_process(decoder, &raw, &out_frame);
//             MY_LOGE("raw->consume = %d",raw.consumed);
//             raw.len -= raw.consumed;
//             raw.buffer += raw.consumed;
//             if (out_frame.len > 0)
//             {
//                 // 播放解码后的数据
//                 bsp_write_dat(out_frame.buffer, out_frame.len);
//             }
//         }
//         vRingbufferReturnItem(my_decoder.in_buf, storge_ptr);
//     }
//     free(out_frame.buffer);
//     vTaskDelete(NULL);

// }