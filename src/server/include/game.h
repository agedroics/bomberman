#ifndef BOMBERMAN_GAME_H
#define BOMBERMAN_GAME_H

#include <stdio.h>
#include "object.h"
#include "packet.h"

#define TIMER 200
#define TICK_RATE 30
#define FILL_SPEED 15
#define FIELD_WIDTH 23
#define FIELD_HEIGHT 23

void setup_game(void);

void reset_game(void);

/**
 * @return game over
 */
int do_tick(uint16_t timer, time_t cur_time);

#endif //BOMBERMAN_GAME_H
