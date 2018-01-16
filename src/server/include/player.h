#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PLAYERS 16
#define PLAYER_SIZE 0.8

#define DEFAULT_POWER 2
#define MAX_POWER 4
#define DEFAULT_COUNT 1
#define MAX_COUNT 3
#define DEFAULT_SPEED 5
#define MAX_SPEED 8

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
} player_t;

extern pthread_mutex_t players_lock;
extern player_t *players;
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
player_t *add_player(int fd, char *name);

/*
 * side effects:
 * deallocated memory
 * item removed from players stack
 * socket closed
 * decreased player_count
 */
void remove_player(player_t *player);

int all_players_ready(void);

void set_players_not_ready(void);

int are_players_nearby(uint8_t x, uint8_t y, uint8_t distance);

int player_intersects(player_t *player, double x, double y);

#endif //BOMBERMAN_PLAYER_H
