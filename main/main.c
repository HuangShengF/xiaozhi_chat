/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ota.h"
#include "bsp_sound.h"
#include "bsp_wifi.h"
#include "audio_sr.h"
#include "audio_encode.h"
#include "audio_decode.h"
#include "Commu_ws.h"
#include "display.h"
#include "bsp_ws2812.h"
#include "bsp_ledc.h"

// edit snfjk

// fu edit 2345678
// 富 edit eirjtg

// 创建缓冲区
typedef struct
{
    RingbufHandle_t sr_2_encode;
    RingbufHandle_t encode_2_ws;
    RingbufHandle_t ws_2_decode;

} buf_t;
buf_t my_buf;

// wifi回调函数,,wifi注册成功底层会调用这个函数
void wifi_secc(void);

// 主函数任务，这个也是FreeRTOS创建的任务
void app_main(void)
{
    bsp_sound_init();
    display_init();
    Bsp_W2812_Init();
    backlight_init();
    bsp_wifi_init(wifi_secc);
    while (1)
    {
        // MY_LOGE("Hello world!\n");
        UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL); // NULL 表示当前任务
        printf("Stack high water mark: %u bytes\n", uxHighWaterMark * sizeof(StackType_t));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void buf_create(void);
void wakeup_cb(void);
void vad_change_cb(vad_state_t state);

void test_task(void *arg);

void upload_task(void *arg);

// 音频处理回调
void audio_broadcast(char *data, size_t size);

void wifi_secc(void)
{
    Myota_check();
    // 必须激活了，才能往下走

    // 创建缓冲区
    buf_create();

    // SR模型初始化
    audio_sr_init();
    audio_sr_start(my_buf.sr_2_encode, wakeup_cb, vad_change_cb);

    // 编码器初始化
    audio_encode_init();
    audio_encode_start(my_buf.sr_2_encode, my_buf.encode_2_ws);

    // 解码器初始化
    audio_decode_init();
    audio_decode_start(my_buf.ws_2_decode);

    // websoket 初始化
    Commu_ws_init();
    audio_register_cb(audio_broadcast);

    // 创建一个上传任务upload_task
    xTaskCreate(upload_task, "upload_task", 4 * 1024, NULL, 5, NULL);
    // xTaskCreatePinnedToCoreWithCaps(upload_task, "upload_task", 4 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);

    // 测试: 麦克->es8311->sr->编码器->环形缓冲区->测试任务->环形缓冲区->解码->es8311->喇叭
    // xTaskCreate(test_task, "test_task", 4096, NULL, 5, NULL);

    // display_show_usage();
}

void buf_create(void)
{
    // 创建缓冲区
    // 这个类型必须是字节流,因为要跟后面的读取字节流一致，编码的时候必须指定字节流大小
    my_buf.sr_2_encode = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);

    // 下面这个缓冲区类型无所谓
    my_buf.encode_2_ws = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    my_buf.ws_2_decode = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
}

void wakeup_cb(void)
{
    // 每次说"你好，小智"，会自动调用这个函数
    MY_LOGE("wakeup_callback");

    switch (xiaozhi_state)
    {
    case XIAOZHI_IDLE:
        // 唤醒后就连接websoket
        if (my_commu_ws.is_connected == false)
        {
            // MY_LOGE("00");
            Commu_ws_start();
            // MY_LOGE("11");
            // 给服务器发送hello包
            commu_ws_send_hello();
            xiaozhi_state = XIAOZHI_CONNECTED;

            // MY_LOGE("22");
            // 等待接收hello包
            xEventGroupWaitBits(my_commu_ws.eg, RECV_HELLO_BIT, true, true, portMAX_DELAY);
        }
        break;
    case XIAOZHI_SPEAKING:
        // 在小智说话期间，如果我说 "你好小智" ，则打断小智让他停止说话
        Commu_ws_abort();
        break;
    default:
        MY_LOGE("xiaozhi state %d", xiaozhi_state);
        break;
    }

    // 发送唤醒词
    commu_ws_send_wakeup();
    xiaozhi_state = XIAOZHI_IDLE;
}

void vad_change_cb(vad_state_t state)
{
    MY_LOGE("%s", (state == VAD_SPEECH) ? "VAD_STATE_SPEECH" : "VAD_STATE_VAD_SILENCE");
    if (state == VAD_SPEECH && xiaozhi_state != XIAOZHI_SPEAKING)
    {
        // 小智开始监听
        Commu_ws_listen();
        xiaozhi_state = XIAOZHI_LISTENING;
    }
    else if (state == VAD_SILENCE && xiaozhi_state == XIAOZHI_LISTENING) // 当我不说话时，让小智停止监听
    {
        Commu_ws_stop_listen();
        xiaozhi_state = XIAOZHI_IDLE;
    }
}

// 把服务器的音频数据，解码后，发送给解码任务
void audio_broadcast(char *data, size_t size)
{
    xRingbufferSend(my_buf.ws_2_decode, data, size, portMAX_DELAY);
}

void upload_task(void *arg)
{
    while (1)
    {
        size_t size;
        char *data = xRingbufferReceive(my_buf.encode_2_ws, &size, portMAX_DELAY);
        // 当我在说话的时候(小智监听的时候)，上传数据，从缓冲区中取出数据上传
        if (xiaozhi_state == XIAOZHI_LISTENING && data && size)
        {
            // 上传我们的opus数据
            Commu_ws_send_audio_data(data, size);
        }
        vRingbufferReturnItem(my_buf.encode_2_ws, data);
    }
}

// void test_task(void *arg)
// {
//     size_t size = 0;
//     while(1)
//     {
//         void *data = xRingbufferReceive(my_buf.encode_2_ws, &size, portMAX_DELAY);
//         if(data && size > 0)
//         {
//             xRingbufferSend(my_buf.ws_2_decode, data, size, portMAX_DELAY);

//             vRingbufferReturnItem(my_buf.encode_2_ws, data);
//         }
//     }
// }
