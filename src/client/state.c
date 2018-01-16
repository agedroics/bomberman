#include "state.h"

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;

uint16_t timer;

uint8_t *field;
uint8_t field_width;
uint8_t field_height;

uint8_t field_get(int x, int y) {
    return field[y * field_width + x];
}

void field_set(int x, int y, uint8_t block_type) {
    field[y * field_width + x] = block_type;
}

char player_names[256][24];
player_t players[256];
uint8_t player_cnt;

dyn_t dynamites[256];
uint8_t dyn_cnt;
uint8_t dyn_timer;

flame_t flames[256];
uint8_t flame_cnt;

pwrup_t pwrups[256];
uint8_t pwrup_cnt;

uint8_t winner_ids[256];
uint8_t winner_cnt;
