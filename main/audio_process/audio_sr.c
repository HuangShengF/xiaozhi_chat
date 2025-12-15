#include "audio_sr.h"

sr_t my_sr = {
    .is_running = false};

esp_afe_sr_iface_t *afe_handle;
// create instance
esp_afe_sr_data_t *afe_data;

void audio_sr_init(void)
{
    srmodel_list_t *models = esp_srmodel_init("model"); // flash名字，，与我们分区表名字要一致
    // 音频前端配置 参数1: 定义通道排列  参数2: 模型 参数3:前端类型(语音识别) 参数4: 前端模式(低消耗模式)
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->wakenet_init = true;
    // afe_config->wakenet_model_name = "wakenet3_infer.bin"; // 这个不需要配置，我们在SDK里面设置了
    afe_config->wakenet_mode = DET_MODE_90; // 唤醒模式: 值越大越容易被唤醒,但是容易误唤醒

    // VAD
    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_1; // vad模式: 活动语音检测的敏感度, 值越大越敏感

    // 其他
    afe_config->aec_init = false; // 回声消除
    afe_config->ns_init = false;  // 噪声抑制
    afe_config->se_init = false;  // 语音 enhancement

    // get handle
    afe_handle = esp_afe_handle_from_config(afe_config);
    // create instance
    afe_data = afe_handle->create_from_config(afe_config);
}

void feed(void *argv);
void fetch(void *argv);
void audio_sr_start(RingbufHandle_t ringbuf,
                    void (*wakeup_cb)(void),
                    void (*vad_change_cb)(vad_state_t state))
{
    my_sr.is_running = true;
    my_sr.is_wakeup = false;
    my_sr.last_vad_state = VAD_SILENCE; // 安静状态

    my_sr.buffer = ringbuf;

    my_sr.wakeup_callback = wakeup_cb;
    my_sr.vad_change_callback = vad_change_cb;

    // 创建两个任务，一个从es8311读取数据放到SR模组，一个从模组拿数据放到缓冲区
    // 创建任务，内存放到外部sram
    xTaskCreateWithCaps(feed, "feed", 4096, NULL, 5, NULL, MALLOC_CAP_SPIRAM);
    xTaskCreateWithCaps(fetch, "fetch", 4096, NULL, 5, NULL, MALLOC_CAP_SPIRAM);
    // xTaskCreate(feed, "feed", 4096, NULL, 5, NULL);
    // xTaskCreate(fetch, "fetch", 4096, NULL, 5, NULL);
}

void feed(void *argv)
{
    // 每帧的样本数(采样数)
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    // 音频的通道数
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    // 创建的buf大小
    int16_t size = feed_chunksize * feed_nch * sizeof(int16_t);

    MY_LOGE("feed_chunksize:%d,feed_nch:%d,size:%d", feed_chunksize, feed_nch, size);
    int16_t *feed_buff = (int16_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    while (my_sr.is_running)
    {
        Doorbell_state sta = bsp_read_dat((uint8_t *)feed_buff, size); // 把从es83111的音频数据读到feed_buff中，此时是PCM格式
        if (sta == COM_UTILT_OK)
        {
            afe_handle->feed(afe_data, feed_buff);
        }
        // vTaskDelay(10);
    }
    MY_LOGE("feed task end");
    free(feed_buff);
    vTaskDelete(NULL);
}

void fetch(void *arg)
{
    MY_LOGE("fetch task start");
    while (my_sr.is_running)
    {
        // 取出sr处理后的结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        int16_t *processed_audio = result->data;
        int16_t processed_audio_len = result->data_size;
        // 说话的状态
        vad_state_t vad_state = result->vad_state;
        // 唤醒的状态
        wakenet_state_t wakeup_state = result->wakeup_state;
        if (wakeup_state == WAKENET_DETECTED)
        {
            // 如果已经唤醒了，需要执行我们传进来的回调函数
            my_sr.wakeup_callback();
            my_sr.is_wakeup = true;
        }

        if (my_sr.is_wakeup) // 如果已经唤醒了，就进行语音识别(语音状态变化，则调用状态改变回调函数)
        {
            if (vad_state != my_sr.last_vad_state) // 这次和上次状态对比
            {
                if(my_sr.vad_change_callback)
                {
                    my_sr.vad_change_callback(vad_state);
                }
                // my_sr.vad_change_callback(vad_state);
                my_sr.last_vad_state = vad_state;
            }
        }

        if (my_sr.is_wakeup && vad_state == VAD_SPEECH) // 如果是正在说话，存放到缓存中buffer中
        {
            // if vad cache is exists, please attach the cache to the front of processed_audio to avoid data loss
            if (result->vad_cache_size > 0) // 这是防止前面的数据丢失，可能说话的时候没反应过来，下面存到了cache,可以把他取出来
            {
                int16_t *vad_cache = result->vad_cache;
                if(my_sr.buffer)
                {
                    xRingbufferSend(my_sr.buffer, vad_cache, result->vad_cache_size, 0); // 这个0表示不等待，如果缓存已满，则丢弃数据
                }
            }
            if(my_sr.buffer)
            {
                xRingbufferSend(my_sr.buffer, processed_audio, processed_audio_len, 0);
            }
        }
    }
    MY_LOGE("SR task exit");
    vTaskDelete(NULL);
}