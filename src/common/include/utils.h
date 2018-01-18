#ifndef BOMBERMAN_UTILS_H
#define BOMBERMAN_UTILS_H

#include <sys/time.h>
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef long long millis_t;

millis_t get_milliseconds(void);

#endif //BOMBERMAN_UTILS_H
