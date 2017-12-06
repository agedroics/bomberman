#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "../protocol.h"

typedef struct player {
    char name[24];
    uint8_t id;
    uint8_t ready;
    uint16_t input;
    uint16_t x;
    uint16_t y;
    uint8_t direction;
    uint8_t frags;
    uint8_t power;
    uint8_t speed;
    uint8_t count;
    uint8_t active_pwrups;
    uint8_t dead;
} player;

/*
 * default: 8
 */
extern int max_players;

/*
 * returns created player struct on success
 * returns NULL if no free slots available
 */
player *add_player(char *name);

/*
 * returns 0 on success
 * returns -1 if id is invalid or player does not exist
 */
int remove_player(int id);

int get_player_count();

void clear_players();

size_t prepare_lobby_status(void **ptr);

#endif //BOMBERMAN_PLAYER_H
