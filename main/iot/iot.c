#include "iot.h"

char *description =
#include "descriptions.txt"
;

char *state = 
#include "states.txt"
;

char *ws2812 =
#include "ws2812.txt"
;

char *display = 
#include "display.txt"
;

char *return_description_txt(void)
{
    return description;
}

char *return_state_txt(void)
{
    return state;
}

char *return_ws2812_txt(void)
{
    return ws2812;
}

char *return_display_txt(void)
{
    return display;
}



