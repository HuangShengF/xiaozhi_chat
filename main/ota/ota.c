#include "ota.h"

#include <stdio.h>
#include <string.h>
#include "esp_system.h" // esp_random()
#include "esp_log.h"

#define OTA_URI "https://api.tenclass.net/xiaozhi/ota/"
#define TAG "OTA"

#define ACTIVE_BIT (1<<0)

ota_t my_ota;
EventGroupHandle_t eg = NULL;

static char *output_buffer; // Buffer to store response of http request from event handler
static int output_len;      // Stores number of bytes read
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        // 获取响应头信息
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcmp(evt->header_key, "Content-Length") == 0)
        {
            if (output_buffer) // 如果存在output_buffer，则释放
            {
                free(output_buffer);
                output_buffer = NULL;
                output_len = 0;
            }
            output_len = 0;
            output_buffer = malloc(atoi(evt->header_value) + 1);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        // 获取响应数据
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%.*s", evt->data_len, (char *)evt->data);
        if (output_buffer)
        {
            memcpy(output_buffer + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }

    return ESP_OK;
}

void ota_task(void *args);
void Myota_check(void)
{
    // 创建一个任务
    BaseType_t ret = xTaskCreate(ota_task, "ota_task", 4 * 1024, NULL, 5, NULL);
    if (ret != pdPASS)
    {
        MY_LOGE("OTA task create failed!");
    }
    else
    {
        MY_LOGE("OTA task create success!");
    }

    MY_LOGE("waiting for device is active");
    // 把设备激活了才继续往下面走，不然会出现问题
    xEventGroupWaitBits(eg, ACTIVE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    MY_LOGE("device is active,code can run following code");

}

// 生成设备ID(MAC)
char *generete_Device_Id(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char *device_id = malloc(18); // MAC是6个字节加上5个：   一共17字节加"\0”,18字节
    sprintf(device_id, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    MY_LOGI("Device-Id: %s", device_id);

    return device_id;
}

// 注意：调用者需负责释放返回的内存！
char *generate_Client_Id(void)
{
    // 先从flash里面读取，如果存在Client_id则取出来（不需要每次都生成）
    nvs_handle_t handle;
    nvs_open("OTA", NVS_READWRITE, &handle);
    size_t out_size = 0;
    nvs_get_str(handle, "uuid", NULL, &out_size);
    if (out_size > 0)
    {
        // 如果存在uuid,从flash拿出来
        char *out = malloc(out_size);
        nvs_get_str(handle, "uuid", out, &out_size);
        MY_LOGI("Client-Id: %s", out);
        nvs_close(handle);
        return out;
    }

    uint8_t uuid[16];
    // 分配 37 字节（36 + '\0'）
    char *out = malloc(37);
    if (!out)
    {
        ESP_LOGE("UUID", "malloc failed");
        return NULL;
    }

    // 用 esp_random() 填充 16 字节（伪随机，但足够用于 client ID）
    for (int i = 0; i < 16; i++)
    {
        uuid[i] = esp_random() & 0xFF;
    }

    // 设置 UUID 版本为 4（第7字节高4位设为 0100）
    uuid[6] = (uuid[6] & 0x0F) | 0x40;

    // 设置变体为 RFC 4122（第9字节高2位设为 10）
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    // 格式化为字符串：8-4-4-4-12
    snprintf(out, 37,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3],
             uuid[4], uuid[5],
             uuid[6], uuid[7],
             uuid[8], uuid[9],
             uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

    // 如果是第一次生成，保存到 flash
    nvs_set_str(handle, "uuid", out); // 这个没有放到flash，只是缓冲区
    nvs_commit(handle);               // 必须调用这个函数才能把他存到flash

    MY_LOGI("Client-Id: %s", out);
    nvs_close(handle);
    return out;
}
// 请求头响应的请求
void send_header_request(esp_http_client_handle_t client)
{
    // HTTP用post，比较安全相较于GET
    esp_http_client_set_method(client, HTTP_METHOD_POST);                   /*!< HTTP POST Method */
    esp_http_client_set_header(client, "Content-Type", "application/json"); // // 添加请求头，cjson 生成
    esp_http_client_set_header(client, "User-Agent", "bread-compact-wifi-128x64/1.0.1");

    char *device_id = generete_Device_Id();
    esp_http_client_set_header(client, "Device-Id", device_id);

    char *client_id = generate_Client_Id();
    esp_http_client_set_header(client, "Client-Id", client_id);

    // strdup:在堆上分配一块新内存，将字符串 s 复制进去，并返回指向该内存的指针。
    my_ota.client_id = strdup(client_id);
    my_ota.device_id = strdup(device_id);

    free(device_id);
    free(client_id);
}

void send_body_request(esp_http_client_handle_t client)
{
    cJSON *root = cJSON_CreateObject();

    cJSON *app = cJSON_CreateObject();

    cJSON_AddStringToObject(app, "version", "1.0.1");
    cJSON_AddStringToObject(app, "elf_sha256", esp_app_get_elf_sha256_str());
    cJSON_AddItemToObject(root, "application", app);

    cJSON *board = cJSON_CreateObject();
    cJSON_AddStringToObject(board, "type", "bread-compact-wifi");
    cJSON_AddStringToObject(board, "name", "bread-compact-wifi-128X64");
    cJSON_AddStringToObject(board, "ssid", "atguigu");
    cJSON_AddNumberToObject(board, "rssi", -55);
    cJSON_AddNumberToObject(board, "channel", 1);
    cJSON_AddStringToObject(board, "ip", "192.168.1.1");
    cJSON_AddStringToObject(board, "mac", my_ota.device_id);

    cJSON_AddItemToObject(root, "board", board);

    char *json = cJSON_PrintUnformatted(root);
    // 设置请求体
    esp_http_client_set_post_field(client, json, strlen(json));

    MY_LOGI("request body: %s", json);
    cJSON_Delete(root);
}
bool ota_is_activated(void)
{
    // 解析JSON 数据
    cJSON *root = cJSON_ParseWithLength(output_buffer, output_len);
    if (root == NULL)
    {
        MY_LOGE("JSON 解析失败");
        return false;
    }
    cJSON *websocket = cJSON_GetObjectItem(root, "websocket");

    char *url = cJSON_GetObjectItem(websocket, "url")->valuestring;
    char *token = cJSON_GetObjectItem(websocket, "token")->valuestring;


    // 如果激活过了，这个activation 字段为空
    cJSON *activeation = cJSON_GetObjectItem(root, "activation");

    my_ota.activate_code = NULL;
    if(activeation) // 如果不为空才赋值
    {
        my_ota.activate_code = strdup(cJSON_GetObjectItem(activeation, "code")->valuestring);
        MY_LOGI("activate_code: %s", my_ota.activate_code);
    }
    // my_ota.activate_code = strdup(code);
    my_ota.token = strdup(token);
    my_ota.ws_url = strdup(url);

    
    MY_LOGI("token: %s", token);
    MY_LOGI("websocket: %s", url);

    cJSON_Delete(root);

    return (my_ota.activate_code == NULL); // 没有激活码，说明已经激活过了
}
void ota_task(void *args)
{
    // 创建事件组
    eg = xEventGroupCreate();


    esp_http_client_config_t config = {
        .url = OTA_URI,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 发送请求头
    //- Device-Id: 设备的唯一标识符（必需，使用MAC地址或由硬件ID生成的伪MAC地址）
    //- Client-Id: 客户端的唯一标识符，由软件自动生成的UUID v4（必需，擦除FLASH或重装后会变化）
    //- User-Agent: 客户端的名字和版本号（必需，例如 esp-box-3/1.5.6）
    send_header_request(client);

    // 请求body
    send_body_request(client);

    while (1)
    {
        // 发送请求,(阻塞式请求: 直到数据收完,才会返回)
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            // 获取请求码
            MY_LOGI("ota request seccee: %d", esp_http_client_get_status_code(client));
            if (esp_http_client_get_status_code(client) == 200) // 200: 请求成功，404: 资源不存在（URI错误）
            {
                MY_LOGI("%.*s", output_len, output_buffer);
                if (ota_is_activated())
                {
                    MY_LOGI("device activated");
                    xEventGroupSetBits(eg, ACTIVE_BIT);
                    break;
                }
                else
                {
                    MY_LOGI("device not activated");
                }
            }

            // MY_LOGE("send request success");
            // MY_LOGI("%.*s", output_len, output_buffer);
        }
        else
        {
            MY_LOGE("send request failed");
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS); // 10秒钟后重新发送请求
    }
    free(output_buffer);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}
