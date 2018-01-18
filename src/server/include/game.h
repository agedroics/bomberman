#ifndef BOMBERMAN_GAME_H
#define BOMBERMAN_GAME_H

#include <stdio.h>
#include <object.h>
#include <packet.h>

#define TIMER 200
#define TICK_RATE 60
#define FILL_SPEED 10
#define FIELD_WIDTH 19
#define FIELD_HEIGHT 19

void setup_game(void);

/**
 * @return game over
 */
int do_tick(uint16_t timer, millis_t cur_time);

#endif //BOMBERMAN_GAME_H
