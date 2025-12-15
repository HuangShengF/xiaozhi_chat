#include "Commu_ws.h"

// 声明函数，函数的实现在下面
static void stt_cb(char *data);
static void tts_cb(TTS_STATE state, char *data);
static void llm_cb(char *text, char *emotion);

// 让喇叭静音
void Commu_ws_audio_mute(bool sta);
// 调节音量
void Commu_ws_audio_set_volume(uint8_t vol);

commu_ws_t my_commu_ws;

void hello_text_process(uint8_t *data, uint16_t size);

static const char *TAG = "Commu_ws";
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
        my_commu_ws.is_connected = false;
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");

        // 这里wsws服务器成功后会进这里，我们搁这里发送通知
        xEventGroupSetBits(my_commu_ws.eg, CONNECTED_BIT);
        my_commu_ws.is_connected = true;
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        my_commu_ws.is_connected = false;
        break;
    case WEBSOCKET_EVENT_DATA:
        my_commu_ws.is_connected = true;
        // ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        // ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x2)
        { // Opcode 0x2 indicates binary data

            // 播放音频
            my_commu_ws.audio_braodcast_cb((char *)data->data_ptr, data->data_len);

            // ESP_LOG_BUFFER_HEX("Received binary data", data->data_ptr, data->data_len);
        }
        else if (data->op_code == 0x08 && data->data_len == 2)
        {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        }
        else if (data->op_code == 0x01) // 服务器给我们发送的文本帧
        {
            // ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
            hello_text_process((uint8_t *)data->data_ptr, data->data_len);
        }
        else
        {
            MY_LOGI("Other opcode------------");
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        my_commu_ws.is_connected = false;
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    case WEBSOCKET_EVENT_FINISH:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH-------");

        my_sr.is_wakeup = false;

        my_commu_ws.is_connected = false;
        break;
    }
}

esp_websocket_client_handle_t client;
void Commu_ws_init(void)
{
    // 创建事件组
    my_commu_ws.eg = xEventGroupCreate();

    esp_websocket_client_config_t websocket_cfg = {
        .uri = my_ota.ws_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 5000,
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL, // 因为小智服务器返回给我们的URI是WSS格式，所以需使用SSL
        .buffer_size = 2 * 1024,
    };
    client = esp_websocket_client_init(&websocket_cfg);
    if (!client)
    {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        return;
    }

    // 1. 客户端连接Websocket服务器时需要携带以下headers:
    // Authorization: Bearer <access_token>
    // Protocol-Version: 1
    // Device-Id: <设备MAC地址>
    // Client-Id: <设备UUID>
    char *auth_header = NULL;
    asprintf(&auth_header, "Bearer %s", my_ota.token);
    esp_websocket_client_append_header(client, "Authorization", auth_header);
    esp_websocket_client_append_header(client, "Protocol-Version", "1");
    esp_websocket_client_append_header(client, "Device-Id", my_ota.device_id);
    esp_websocket_client_append_header(client, "Client-Id", my_ota.client_id);

    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    free(auth_header);
}

// 开始连接服务器，建立websocket连接,在唤醒的时候才调用
void Commu_ws_start(void)
{
    // 如果没有建立连接，则进行连接
    if (client && !esp_websocket_client_is_connected(client))
    {
        esp_websocket_client_start(client);
        // 等待连接成功，如何判断呢----》在回调函数中有成功连接的事件，，在哪里发送事件通知，我们在这里死等
        xEventGroupWaitBits(my_commu_ws.eg, CONNECTED_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    }
}

static void commu_ws_send_text(char *text)
{
    if (client && esp_websocket_client_is_connected(client) && strlen(text) > 0)
    {
        esp_websocket_client_send_text(client, text, strlen(text), portMAX_DELAY);
    }
}

// 终止消息，打断小智的对话
void Commu_ws_abort(void)
{
    // 组装成json格式
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "abort");
    cJSON_AddStringToObject(root, "reason", "wake_word_detected");
    char *json = cJSON_Print(root);
    commu_ws_send_text(json);
    cJSON_free(json);
    cJSON_Delete(root);
}

// 小智开始监听
void Commu_ws_listen(void)
{
    // 组装成json格式
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "start");
    cJSON_AddStringToObject(root, "mode", "manual");
    char *json = cJSON_Print(root);
    commu_ws_send_text(json);
    cJSON_free(json);
    cJSON_Delete(root);
}

// 小智停止监听
void Commu_ws_stop_listen(void)
{
    // 组装成json格式
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "stop");
    char *json = cJSON_Print(root);
    commu_ws_send_text(json);

    cJSON_free(json);
    cJSON_Delete(root);
}

// 上传我们的音频数据（opus格式）
void Commu_ws_send_audio_data(char *data, size_t size)
{
    if (client && data && size && my_commu_ws.is_connected)
    {
        esp_websocket_client_send_bin(client, data, size, portMAX_DELAY);
    }
}

// 2. 连接成功后，客户端发送hello消息:发送hellow文本
void commu_ws_send_hello(void)
{
    // 组装成json格式
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", 1);
    cJSON_AddStringToObject(root, "transport", "websocket");

    cJSON *features = cJSON_CreateObject();
    cJSON_AddBoolToObject(features, "mcp", true);
    cJSON_AddItemToObject(root, "features", features);

    cJSON *audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", "opus");
    cJSON_AddNumberToObject(audio_params, "sample_rate", 16000);
    cJSON_AddNumberToObject(audio_params, "channels", 1);
    cJSON_AddNumberToObject(audio_params, "frame_duration", 60);
    cJSON_AddItemToObject(root, "audio_params", audio_params);

    // 把创建的JSON对象转换成字符串
    char *json_str = cJSON_PrintUnformatted(root); // 创建一个无格式的JSON字符串（节省空间），发送快

    commu_ws_send_text(json_str);

    cJSON_free(json_str);
    cJSON_Delete(root);
}

// 发送我们设备IOT
void commu_ws_send_iot_description(void)
{
    commu_ws_send_text(return_description_txt());
}
// 发送设备状态
void commu_ws_send_iot_state(void)
{
    commu_ws_send_text(return_state_txt());
}
// 发送WS2812设备
void commu_ws_send_ws2812_description(void)
{
    commu_ws_send_text(return_ws2812_txt());
}

// 发送屏幕参数
void commu_ws_send_display(void)
{
    commu_ws_send_text(return_display_txt());
}

// 唤醒小智
void commu_ws_send_wakeup(void)
{
    // 组装成json格式
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "state", "detect");
    cJSON_AddStringToObject(root, "text", "你好,小智");

    char *json_str = cJSON_PrintUnformatted(root);

    commu_ws_send_text(json_str);

    cJSON_free(json_str);
    cJSON_Delete(root);
}

// 服务器给我们发送数据，因为我们给他发了hello 文本
void hello_text_process(uint8_t *data, uint16_t size)
{
    MY_LOGI("websocket receive buf:%.*s", size, data);

    // 对cjson解析
    cJSON *root = cJSON_Parse((char *)data);

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (type != NULL)
    {
        if (strcmp(type->valuestring, "hello") == 0) // 接收到hello,说明我们可以给大模型小智发数据了
        {
            // 在这里发送一个通知
            xEventGroupSetBits(my_commu_ws.eg, RECV_HELLO_BIT);

            // 发送描述信息,IOT描述信息
            commu_ws_send_iot_description();
            commu_ws_send_iot_state();
            commu_ws_send_ws2812_description();
            commu_ws_send_display();
        }
        else if (strcmp(type->valuestring, "tts") == 0)
        {
            cJSON *state = cJSON_GetObjectItem(root, "state");
            cJSON *text = cJSON_GetObjectItem(root, "text");
            if (text == NULL)
            {
                MY_LOGI("tts_text is null");
            }
            if (state != NULL)
            {
                if (strcmp(state->valuestring, "start") == 0)
                {
                    tts_state = TTS_START;
                }
                else if (strcmp(state->valuestring, "stop") == 0)
                {
                    tts_state = TTS_STOP;
                }
                else if (strcmp(state->valuestring, "sentence_start") == 0)
                {
                    tts_state = TTS_SENTENCE_START;
                }
                else if (strcmp(state->valuestring, "sentence_stop") == 0)
                {
                    tts_state = TTS_SENTENCE_STOP;
                }

                // 这个tts可能为空
                tts_cb(tts_state, text ? text->valuestring : "");
            }
        }
        else if (strcmp(type->valuestring, "stt") == 0)
        {
            cJSON *text = cJSON_GetObjectItem(root, "text");
            if (text)
            {
                // MY_LOGD("stt: %s", text->valuestring);
                stt_cb(text->valuestring);
            }
            else
            {
                MY_LOGD("stt: null");
            }
        }
        else if (strcmp(type->valuestring, "llm") == 0)
        {
            cJSON *text = cJSON_GetObjectItem(root, "text");
            cJSON *emotion = cJSON_GetObjectItem(root, "emotion");
            if (text && emotion)
            {
                // MY_LOGD("llm: %s", text->valuestring);
                llm_cb(text->valuestring, emotion->valuestring);
            }
            else
            {
                MY_LOGD("llm: null");
            }
        }

        else if (strcmp(type->valuestring, "iot") == 0)
        {
            cJSON *commands = cJSON_GetObjectItem(root, "commands");

            // 检查 commands 是否为数组
            if (!cJSON_IsArray(commands))
            {
                MY_LOGE("commands is not an array");
                return;
            }

            // 取第一个命令对象
            cJSON *cmd = cJSON_GetArrayItem(commands, 0);
            if (!cmd)
            {
                MY_LOGE("commands array is empty");
                return;
            }

            // 从 cmd 对象中获取 method 字段
            cJSON *method_item = cJSON_GetObjectItem(cmd, "method");
            if (!method_item || !cJSON_IsString(method_item))
            {
                MY_LOGE("method field missing or not string");
                return;
            }

            const char *method = method_item->valuestring;
            MY_LOGE("method: %s", method);

            if (strcmp(method, "SetMute") == 0)
            {
                // 从 cmd 中获取 parameters
                cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
                if (!params)
                {
                    MY_LOGE("parameters missing");
                    return;
                }

                cJSON *mute_item = cJSON_GetObjectItem(params, "mute");
                bool mute_value = cJSON_IsTrue(mute_item);
                MY_LOGD("mute = %s", mute_value ? "true" : "false");
                Commu_ws_audio_mute(mute_value);
            }
            else if (strcmp(method, "SetVolume") == 0)
            {
                cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
                if (!params)
                {
                    MY_LOGE("parameters missing");
                    return;
                }

                cJSON *volume_item = cJSON_GetObjectItem(params, "volume");
                if (!volume_item || !cJSON_IsNumber(volume_item))
                {
                    MY_LOGE("volume missing or not number");
                    return;
                }

                int volume = volume_item->valueint;
                MY_LOGD("volume = %d", volume);
                bsp_sound_set_volume(volume);
            }
            // ===== 新增：WS2812 灯带控制 =====
            else if (strcmp(method, "TurnOn") == 0)
            {
                MY_LOGD("WS2812: TurnOn");
                uint8_t color[3] = {255, 255, 255};
                Bsp_W2812_Set_Color_ON(color); // 调用你的开关灯函数
            }
            else if (strcmp(method, "TurnOff") == 0)
            {
                MY_LOGD("WS2812: TurnOff");
                Bsp_W2812_OFF(); // 调用你的关灯函数
            }
            else if (strcmp(method, "SetColor") == 0)
            {
                cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
                if (!params)
                {
                    MY_LOGE("SetColor: parameters missing");
                    return;
                }

                cJSON *r_item = cJSON_GetObjectItem(params, "r");
                cJSON *g_item = cJSON_GetObjectItem(params, "g");
                cJSON *b_item = cJSON_GetObjectItem(params, "b");

                if (!r_item || !g_item || !b_item ||
                    !cJSON_IsNumber(r_item) || !cJSON_IsNumber(g_item) || !cJSON_IsNumber(b_item))
                {
                    MY_LOGE("SetColor: r/g/b missing or not number");
                    return;
                }

                int r = r_item->valueint;
                int g = g_item->valueint;
                int b = b_item->valueint;

                MY_LOGD("SetColor: R=%d, G=%d, B=%d", r, g, b);
                uint8_t color[3] = {r, g, b};
                Bsp_W2812_Set_Color_ON(color); // 调用你的设色函数
            }
            else if (strcmp(method, "SetBrightness") == 0)
            {
                cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
                if (!params)
                {
                    MY_LOGE("SetBrightness: parameters missing");
                    return;
                }

                cJSON *name_item = cJSON_GetObjectItem(cmd, "name");
                if (!name_item || !cJSON_IsString(name_item) || strcmp(name_item->valuestring, "Display") != 0)
                {
                    MY_LOGE("SetBrightness: name not 'Display'");
                    return;
                }

                cJSON *brightness_item = cJSON_GetObjectItem(params, "brightness");
                if (!brightness_item || !cJSON_IsNumber(brightness_item))
                {
                    MY_LOGE("SetBrightness: brightness missing or not number");
                    return;
                }

                int brightness = brightness_item->valueint;
                if (brightness < 0)
                    brightness = 0;
                if (brightness > 100)
                    brightness = 100;

                MY_LOGD("Set screen brightness to %d%%", brightness);
                // 假设你有函数：bsp_display_set_brightness(int level)
                backlight_set_brightness(brightness);
            }
            // ==============================
        }
        // else if (strcmp(type->valuestring, "iot") == 0)
        // {
        //     cJSON *commands = cJSON_GetObjectItem(root, "commands");
        //     // 取第一个命令对象
        //     cJSON *cmd = cJSON_GetArrayItem(commands, 0);
        //     // 从 cmd 对象中获取 method 字段
        //     cJSON *method_item = cJSON_GetObjectItem(cmd, "method");
        //     const char *method = method_item->valuestring;
        //     if (strcmp(method, "SetMute") == 0)
        //     {
        //         // 从 cmd（不是 commands！）中获取 parameters
        //         cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
        //         cJSON *mute_item = cJSON_GetObjectItem(params, "mute");
        //         bool mute_value = cJSON_IsTrue(mute_item); // 安全：即使 mute_item 为 NULL，cJSON_IsTrue 返回 false
        //         Commu_ws_audio_mute(mute_value);
        //     }
        //     else if (strcmp(method, "SetVolume") == 0)
        //     {
        //         cJSON *params = cJSON_GetObjectItem(cmd, "parameters");
        //         cJSON *volume_item = cJSON_GetObjectItem(params, "volume");
        //         int volume = volume_item->valueint;
        //         MY_LOGD("volume = %d", volume);
        //         bsp_sound_set_volume(volume);
        //     }
        // }
        // else if (strcmp(type->valuestring, "iot") == 0)
        // {

        //     cJSON *commands = cJSON_GetObjectItem(root, "commands");
        //     // 上面这个command是个数组，数组本身 没有字段叫 "method"所以 cJSON_GetObjectItem(commands, "method") 返回 NULL；
        //     // char *method = cJSON_GetObjectItem(commands, 0);
        //     char *method =  cJSON_GetArrayItem(commands, 0)->valuestring;
        //     if(method)
        //     {
        //         MY_LOGE("method:%s", method);
        //     }
        //     else
        //     {
        //         MY_LOGD("method: null");
        //     }

        //     if (strcmp(method, "SetMute") == 0)
        //     {
        //         cJSON *params = cJSON_GetObjectItem(commands, "parameters");
        //         // char *mute = cJSON_GetObjectItem(params, "mute")->type;
        //         bool mute_value = cJSON_IsTrue(cJSON_GetObjectItem(params, "mute")); // 返回 true 或 false
        //         printf("mute = %s\n", mute_value ? "true" : "false");
        //         Commu_ws_audio_mute(mute_value);
        //     }
        //     else if(strcmp(method, "SetVolume") == 0)
        //     {
        //         cJSON *params = cJSON_GetObjectItem(commands, "parameters");
        //         int volume = cJSON_GetObjectItem(params, "volume")->valueint;
        //         printf("volume = %d\n", volume);
        //         bsp_sound_set_volume(volume);
        //     }
        //     // if(strcmp(commands->valuestring, "reboot") == 0)
        // }
    }
    cJSON_Delete(root);
}

void audio_register_cb(void (*cb)(char *, size_t))
{
    my_commu_ws.audio_braodcast_cb = cb;
}

// 语音转文本，，这是小智服务器给我们发送的
static void stt_cb(char *data)
{
    if (data)
    {
        MY_LOGI("stt_cb:%s", data);
    }
    else
    {
        MY_LOGI("stt_cb:NULL");
    }
}

// 文本转语音，这是小智服务器给我们发送的
static void tts_cb(TTS_STATE state, char *data)
{
    if (state == TTS_START)
    {
        xiaozhi_state = XIAOZHI_SPEAKING;
    }
    else if (state == TTS_STOP)
    {
        xiaozhi_state = XIAOZHI_IDLE;
    }
    else if (state == TTS_SENTENCE_START)
    {
        if (data)
        {
            // display_show_tts_stt(data);
            MY_LOGI("%s", data);
        }
        else
        {
            MY_LOGI("tts_cb:NULL");
        }
    }
}

static void llm_cb(char *text, char *emotion)
{
    // display_show_llm(emotion);

    MY_LOGI("llm_text:%s", text);
    MY_LOGI("llm_emotion:%s", emotion);
}

// 让喇叭静音
void Commu_ws_audio_mute(bool sta)
{
    bsp_sound_mute(sta);
    // bsp_write_dat(NULL, 0);
}

// 调节音量
void Commu_ws_audio_set_volume(uint8_t vol)
{
    bsp_sound_set_volume(vol);
}
