idf_component_register(SRCS 

                        "./main.c"
                        "./bsp/bsp_sound.c"
                        "./bsp/bsp_wifi.c"
                        "./bsp/bsp_ws2812.c"
                        "./bsp/bsp_ledc.c"
                        "./Com/Com_utilt.c"
                        "./Com/Com_Debug.c"
                        "./ota/ota.c"
                        "./audio_process/audio_sr.c"
                        "./audio_process/audio_encode.c"
                        "./audio_process/audio_decode.c"
                        "./Commu/Commu_ws.c"
                        "./iot/iot.c"
                        "./display/display.c"

                    INCLUDE_DIRS
                     "./"
                     "./Com"
                     "./bsp"
                     "./ota"
                     "./iot"
                     "./Commu"
                     "./audio_process"
                      "./display"
                     )
