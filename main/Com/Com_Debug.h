#ifndef __COM_DEBUG_H
#define __COM_DEBUG_H

#include "esp_log.h"
#include "string.h"
#include "esp_task.h"

// 调试开关：1=开启日志，0=关闭所有日志
#define DEBUG 1

#if (DEBUG == 1)
// 用局部数组替代全局变量，避免多线程串扰（栈空间足够，无需担心溢出）
#define MY_LOGE(format, ...) do { \
    char tag[32]; /* 缩小数组大小，仅存文件名+行号，节省空间 */ \
    /* __FILE__ 取文件名（不含路径），__LINE__ 取行号 */ \
    snprintf(tag, sizeof(tag), "[%s:%d]", strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__); \
    ESP_LOGE(tag, format, ##__VA_ARGS__); \
} while(0)

#define MY_LOGW(format, ...) do { \
    char tag[32]; \
    snprintf(tag, sizeof(tag), "[%s:%d]", strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__); \
    ESP_LOGW(tag, format, ##__VA_ARGS__); \
} while(0)

#define MY_LOGI(format, ...) do { \
    char tag[32]; \
    snprintf(tag, sizeof(tag), "[%s:%d]", strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__); \
    ESP_LOGI(tag, format, ##__VA_ARGS__); \
} while(0)

#define MY_LOGD(format, ...) do { \
    char tag[32]; \
    snprintf(tag, sizeof(tag), "[%s:%d]", strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__); \
    ESP_LOGD(tag, format, ##__VA_ARGS__); \
} while(0)

#define MY_LOGV(format, ...) do { \
    char tag[32]; \
    snprintf(tag, sizeof(tag), "[%s:%d]", strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__); \
    ESP_LOGV(tag, format, ##__VA_ARGS__); \
} while(0)

#else
// 关闭日志时，宏定义为空（不生成任何代码，不占用Flash/RAM）
#define MY_LOGE(format, ...)
#define MY_LOGW(format, ...)
#define MY_LOGI(format, ...)
#define MY_LOGD(format, ...)
#define MY_LOGV(format, ...)
#endif

#endif // __COM_DEBUG_H