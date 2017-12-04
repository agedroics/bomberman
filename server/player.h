#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "../common/protocol.h"

typedef struct player {
    char name[24];
    uint8_t ready;
    uint8_t dead;
    uint16_t input;
    uint16_t x;
    uint16_t y;
    uint8_t direction;
    uint8_t frags;
    uint8_t power;
    uint8_t speed;
    uint8_t count;
    uint8_t active_pwrups;
} player;

extern pthread_mutex_t players_lock;
extern player *players;
extern char *taken_slots;
extern int player_count;
extern int max_players;

int add_player(char *name);

void clear_players();

#endif //BOMBERMAN_PLAYER_H
