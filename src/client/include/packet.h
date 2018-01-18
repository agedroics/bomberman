#ifndef BOMBERMAN_PACKET_H
#define BOMBERMAN_PACKET_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <protocol.h>
#include <state.h>
#include <reader.h>
#include <utils.h>

/**
 * @return error ? -1 : 0
 */
int send_join_request(int fd, char *nickname);

int send_keep_alive(int fd, uint8_t id);

int send_ready(int fd, uint8_t id);

int send_input(int fd, uint8_t id, uint16_t input);

int send_disconnect(int fd, uint8_t id);

int parse_lobby_status(reader_t *reader);

int parse_game_start(reader_t *reader);

int parse_map_update(reader_t *reader);

int parse_objects(reader_t *reader);

int parse_game_over(reader_t *reader);

#endif //BOMBERMAN_PACKET_H
