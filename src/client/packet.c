#include "packet.h"

int send_join_request(int fd, char *nickname) {
    char msg[24];
    msg[0] = JOIN_REQUEST;
    strncpy(msg + 1, nickname, 23);
    return write(fd, msg, 24) == -1 ? -1 : 0;
}

int send_keep_alive(int fd, uint8_t id) {
    char msg[2];
    msg[0] = KEEP_ALIVE;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}

int send_ready(int fd, uint8_t id) {
    char msg[2];
    msg[0] = READY;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}

int send_input(int fd, uint8_t id, uint16_t input) {
    char msg[4];
    msg[0] = INPUT;
    msg[1] = id;
    memcpy(msg + 2, &input, 2);
    return write(fd, msg, 4) == -1 ? -1 : 0;
}

int send_disconnect(int fd, uint8_t id) {
    char msg[2];
    msg[0] = DISCONNECT;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}
