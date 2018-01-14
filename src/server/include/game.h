#ifndef BOMBERMAN_GAME_H
#define BOMBERMAN_GAME_H

#include "object.h"
#include "packet.h"

#define TICK_RATE 30
#define TIMER 120
#define FILL_SPEED 4

void setup_game(void);

void reset_game(void);

/**
 * @return game over
 */
int do_tick(uint16_t timer, time_t cur_time);

#endif //BOMBERMAN_GAME_H
