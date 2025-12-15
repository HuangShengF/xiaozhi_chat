#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H
 
#include "Com_Debug.h"
#include "Com_utilt.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_enc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_heap_caps.h"

typedef struct {
  bool is_running;

  RingbufHandle_t in_buf;
  RingbufHandle_t out_buf;
} audio_encode_t;

void audio_encode_init(void);
void audio_encode_start(RingbufHandle_t in_buf, RingbufHandle_t out_buf);
#endif // AUDIO_ENCODE_H
