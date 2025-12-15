#ifndef BSP_SOUND_H
#define BSP_SOUND_H
 
#include "driver/gpio.h"
#include <string.h>
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#include "esp_private/rtc_clk.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "driver/i2c_master.h"
#include "Com_Debug.h"
#include "Com_utilt.h"

Doorbell_state bsp_write_dat(uint8_t *data, uint16_t size);
Doorbell_state bsp_read_dat(uint8_t *data, uint16_t size);
void bsp_sound_init(void);
void bsp_sound_mute(bool sta);
void bsp_sound_set_volume(uint8_t vol);

#endif // BSP_SOUND_H


