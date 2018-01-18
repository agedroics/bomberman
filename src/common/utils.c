#include <utils.h>

millis_t get_milliseconds(void) {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (millis_t) time.tv_sec * 1000 + time.tv_usec / 1000;
}
