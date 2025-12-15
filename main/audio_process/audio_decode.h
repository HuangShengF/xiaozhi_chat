#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "Com_Debug.h"
#include "Com_utilt.h"

#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_heap_caps.h"
#include "bsp_sound.h"

typedef struct {
    RingbufHandle_t in_buf;
    bool is_running;
} audio_decode_t;


void audio_decode_init(void);
void audio_decode_start(RingbufHandle_t in_buf);
#endif // AUDIO_DECODE_H
