#ifndef BOMBERMAN_PACKET_H
#define BOMBERMAN_PACKET_H

#include <sys/socket.h>
#include "object.h"

/**
 * @return success
 */
int send_msg(int fd, void *msg, size_t size);

void send_lobby_status(void);

void send_game_start(uint8_t *field, uint8_t w, uint8_t h);

void send_game_over(void);

void send_objects(uint16_t timer);

void send_map_update(void);

#endif //BOMBERMAN_PACKET_H
