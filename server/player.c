#include "player.h"

static pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
static player *players = NULL;
static uint8_t max_id = 0;
int player_count = 0;
int max_players = 8;

void send_lobby_ready() {
    size_t size = 1 + 24 * (size_t) player_count;
    uint8_t *msg = calloc(size, 1);
    msg[0] = LOBBY_STATUS;
    int i;
    player *it;
    for (i = 0, it = players; i < player_count && it; ++i, it = it->next) {
        memcpy(msg + 1 + i * 24, it->name, 23);
        msg[(i + 1) * 24] = it->ready;
    }
    broadcast(msg, size);
    free(msg);
}

player *add_player(int fd, char *name) {
    if (player_count + 1 > max_players) {
        return NULL;
    }
    player *player = calloc(1, sizeof(struct player));
    player->prev = NULL;
    player->next = players;
    if (players) {
        players->prev = player;
    } else {
        players = player;
    }
    player->fd = fd;
    strcpy(player->name, name);
    player->id = max_id++;
    player->power = 1;
    player->speed = 1;
    player->count = 1;
    ++player_count;
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(int)) == -1) {
        fprintf(stderr, "Failed to set socket options: %s\n", strerror(errno));
    }
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
    player_count--;
    close(player->fd);
    free(player);
}

void broadcast(void *msg, size_t size) {
    int i;
    player *it;
    for (i = 0, it = players; i < player_count && it; ++i, it = it->next) {
        // maybe MSG_DONTWAIT
        send(it->fd, msg, size, NULL);
    }
}

void lock_players() {
    pthread_mutex_lock(&players_lock);
}

void unlock_players() {
    pthread_mutex_unlock(&players_lock);
}