#ifndef COM_UTILT_H
#define COM_UTILT_H
 
typedef enum
{
    COM_UTILT_OK = 0,
    COM_UTILT_ERROR,
    COM_UTILT_TIMEOUT,
}Doorbell_state;

typedef enum
{
    esp_2_client = 0,
    client_2_esp
}tansimit_dir;


typedef enum
{
    XIAOZHI_IDLE = 0,
    XIAOZHI_SPEAKING,
    XIAOZHI_LISTENING,
    XIAOZHI_CONNECTED
}XIAOZHI_STATE;
extern XIAOZHI_STATE xiaozhi_state;


typedef enum
{
    TTS_START = 0,
    TTS_STOP,
    TTS_SENTENCE_START,
    TTS_SENTENCE_STOP
}TTS_STATE;
extern TTS_STATE tts_state;

#endif // COM_UTILT_H
