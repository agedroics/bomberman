#include "include/player.h"

static pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t max_id = 0;
player *players = NULL;
uint8_t player_count = 0;

player *add_player(int fd, char *name) {
    if (player_count + 1 > MAX_PLAYERS) {
        return NULL;
    }
    player *player = calloc(1, sizeof(struct player));
    player->prev = NULL;
    player->next = players;
    if (players) {
        players->prev = player;
    }
    players = player;
    player->fd = fd;
    strncpy(player->name, name, 23);
    player->id = max_id++;
    player->power = 1;
    player->speed = PLAYER_SPEED;
    player->count = 1;
    player->max_count = 1;
    ++player_count;
    return player;
}

void remove_player(player *player) {
    if (!player) {
        return;
    }
    if (player->prev) {
        player->prev->next = player->next;
    }
    if (player->next) {
        player->next->prev = player->prev;
    }
    if (player == players) {
        players = player->next;
    }
    --player_count;
    close(player->fd);
    free(player);
}

int all_players_ready(void) {
    player *it;
    for (it = players; it; it = it->next) {
        if (!it->ready) {
            return 0;
        }
    }
    return 1;
}

void set_players_not_ready(void) {
    player *it;
    for (it = players; it; it = it->next) {
        it->ready = 0;
    }
}

void lock_players(void) {
    pthread_mutex_lock(&players_lock);
}

void unlock_players(void) {
    pthread_mutex_unlock(&players_lock);
}
