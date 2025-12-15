#ifndef AUDIO_SR_H
#define AUDIO_SR_H
 
#include "Com_Debug.h"
#include "Com_utilt.h"
#include "esp_afe_sr_models.h"
#include "esp_heap_caps.h"
#include "bsp_sound.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "esp_afe_sr_iface.h"
typedef struct
{
    bool is_running;
    bool is_wakeup;
    vad_state_t last_vad_state;
    RingbufHandle_t buffer;

    void (*wakeup_callback)(void);
    void (*vad_change_callback)(vad_state_t state);

}sr_t;
extern sr_t my_sr;

void audio_sr_init(void);

void audio_sr_start(RingbufHandle_t ringbuf,
                    void (*wakeup_cb)(void),
                    void (*vad_change_cb)(vad_state_t state));
#endif // AUDIO_SR_H
