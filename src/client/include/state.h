#ifndef BOMBERMAN_OBJECT_H
#define BOMBERMAN_OBJECT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

extern pthread_mutex_t state_lock;

extern uint16_t timer;

extern uint8_t *field;
extern uint8_t field_width;
extern uint8_t field_height;

uint8_t field_get(int x, int y);

void field_set(int x, int y, uint8_t block_type);

typedef struct player {
    uint8_t id;
    uint8_t ready;
    uint16_t x;
    uint16_t y;
    uint8_t direction;
    uint8_t active_pwrups;
    uint8_t power;
    uint8_t speed;
    uint8_t count;
    uint8_t dead;
} player_t;

extern char player_names[256][24];
extern player_t players[256];
extern uint8_t player_cnt;

typedef struct dyn {
    uint16_t x;
    uint16_t y;
} dyn_t;

extern dyn_t dynamites[256];
extern uint8_t dyn_cnt;
extern uint8_t dyn_timer;

typedef struct flame {
    uint8_t x;
    uint8_t y;
} flame_t;

extern flame_t flames[256];
extern uint8_t flame_cnt;

typedef struct pwrup {
    uint8_t x;
    uint8_t y;
    uint8_t type;
} pwrup_t;

extern pwrup_t pwrups[256];
extern uint8_t pwrup_cnt;

extern uint8_t winner_ids[256];
extern uint8_t winner_cnt;

#endif //BOMBERMAN_OBJECT_H
