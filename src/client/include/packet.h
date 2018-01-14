#ifndef BOMBERMAN_PACKET_H
#define BOMBERMAN_PACKET_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "protocol.h"

int send_join_request(int fd, char *nickname);

int send_keep_alive(int fd, uint8_t id);

int send_ready(int fd, uint8_t id);

int send_input(int fd, uint8_t id, uint16_t input);

int send_disconnect(int fd, uint8_t id);

#endif //BOMBERMAN_PACKET_H
