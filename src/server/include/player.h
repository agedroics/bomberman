#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PLAYERS 8
#define PLAYER_SIZE 0.8
#define PLAYER_SPEED 3

#define MAX_POWER 5
#define MAX_COUNT 3
#define MAX_SPEED 6

typedef struct player {
    struct player *prev;
    struct player *next;
    int fd;
    char name[24];
    uint8_t id;
    uint8_t ready;
    uint16_t input;
    double x;
    double y;
    uint8_t direction;
    uint8_t frags;
    uint8_t power;
    uint8_t speed;
    uint8_t count;
    uint8_t max_count;
    uint8_t active_pwrups;
    uint8_t dead;
    int plant_pressed;
    int detonate_pressed;
    int pick_up_pressed;
    int carrying_dyn;
} player;

extern player *players;
extern uint8_t player_count;

/*
 * returns created player struct on success
 * returns NULL if no free slots available
 *
 * side effects:
 * allocated memory
 * item added to players stack
 * increased player_count
 * set socket options
 */
player *add_player(int fd, char *name);

/*
 * side effects:
 * deallocated memory
 * item removed from players stack
 * socket closed
 * decreased player_count
 */
void remove_player(player *player);

void lock_players(void);

void unlock_players(void);

int all_players_ready(void);

void set_players_not_ready(void);

#endif //BOMBERMAN_PLAYER_H
