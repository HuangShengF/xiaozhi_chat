#ifndef BSP_WIFI_H
#define BSP_WIFI_H

#include "Com_utilt.h"
#include "Com_Debug.h"
#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h" // 用于生成二维码


#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"


void bsp_wifi_init(void (*wifi_succ_cb)(void));

#endif // BSP_WIFI_H


