#ifndef COMMU_WS_H
#define COMMU_WS_H
 
#include "Com_Debug.h"
#include "Com_utilt.h"
#include <esp_websocket_client.h>
#include "ota.h"
#include "cJSON.h"
#include "string.h"
#include "stdio.h"
#include "audio_sr.h"
#include "iot.h"
#include "bsp_sound.h"
#include "display.h"
#include "bsp_ws2812.h"
#include "bsp_ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"

typedef struct {
    EventGroupHandle_t eg;
    void(* audio_braodcast_cb)(char *data, size_t size);
    bool is_connected;
}commu_ws_t;

#define CONNECTED_BIT (1 << 0)
#define RECV_HELLO_BIT (1 << 1)

extern commu_ws_t my_commu_ws;

void Commu_ws_init(void);
void Commu_ws_start(void);
void commu_ws_send_hello(void);
void commu_ws_send_wakeup(void);
void audio_register_cb(void (*cb)(char *, size_t ));
void Commu_ws_abort(void);

void Commu_ws_send_audio_data(char *data, size_t size);
// 小智开始监听
void Commu_ws_listen(void);

// 小智停止监听
void Commu_ws_stop_listen(void);

#endif // COMMU_WS_H
