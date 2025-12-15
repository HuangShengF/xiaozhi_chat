#ifndef OTA_H
#define OTA_H
 
#include "Com_Debug.h"
#include "Com_utilt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "esp_mac.h"
#include <malloc.h>
#include "nvs.h"
#include "cJSON.h"
#include "esp_app_desc.h"

typedef struct
{
    char *device_id;
    char *client_id;

    char *ws_url;
    char *token;

    char *activate_code;
} ota_t;

extern ota_t my_ota;


void Myota_check(void);

#endif // OTA_H
