#ifndef BOMBERMAN_PLAYER_H
#define BOMBERMAN_PLAYER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include "../protocol.h"

typedef struct player {
    struct player *prev;
    struct player *next;
    int fd;
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

extern int player_count;

// default: 8
extern int max_players;

void send_lobby_ready();

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

void broadcast(void *msg, size_t size);

void lock_players();

void unlock_players();

#endif //BOMBERMAN_PLAYER_H
