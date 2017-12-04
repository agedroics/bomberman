#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "../common/protocol.h"

typedef struct player player;

extern pthread_mutex_t players_lock;
extern player          *players;
extern int             player_count;
extern int             max_players;

int add_player(char *name);

void clear_players();

#endif //BOMBERMAN_PLAYER_H
