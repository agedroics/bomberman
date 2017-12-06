#include "player.h"

static pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
static player *players = NULL;
static char *taken_slots = NULL;
static int player_count = 0;
int max_players = 8;

void create_player(int id, char *name) {
    player *player = players + id;
    player->ready = 0;
    player->dead = 0;
    player->input = 0;
    player->direction = DIRECTION_UP;
    player->frags = 0;
    player->power = 1;
    player->speed = 1;
    player->count = 1;
    player->active_pwrups = 0;
    strcpy(player->name, name);
}

player *add_player(char *name) {
    pthread_mutex_lock(&players_lock);
    if (!players) {
        players = malloc(max_players * sizeof(void *));
        taken_slots = calloc((size_t) max_players, sizeof(char));
    }
    int id = -1;
    int i;
    for (i = 0; i < max_players; ++i) {
        if (!taken_slots[i]) {
            id = i;
            break;
        }
    }
    if (i > -1) {
        create_player(id, name);
        taken_slots[id] = 1;
        ++player_count;
    }
    pthread_mutex_unlock(&players_lock);
    return id == -1 ? NULL : players + id;
}

int remove_player(int id) {
    if (id <= max_players || id < 0 || !taken_slots[id]) {
        return -1;
    }
    pthread_mutex_lock(&players_lock);
    taken_slots[id] = 0;
    player_count--;
    pthread_mutex_unlock(&players_lock);
    return 0;
}

int get_player_count() {
    return player_count;
}

void clear_players() {
    pthread_mutex_lock(&players_lock);
    if (players) {
        free(players);
        free(taken_slots);
        players = NULL;
        taken_slots = NULL;
    }
    pthread_mutex_unlock(&players_lock);
}

size_t prepare_lobby_status(void **ptr) {
    pthread_mutex_lock(&players_lock);
    size_t size = (size_t) 1 + 24 * player_count;
    uint8_t *msg = malloc(size);
    msg[0] = LOBBY_STATUS;
    int i;
    for (i = 0; i < max_players; ++i) {
        if (taken_slots[i]) {
            memcpy(msg + 1 + i * 24, players[i].name, 23);
            msg[(i + 1) * 24] = players[i].ready;
        }
    }
    pthread_mutex_unlock(&players_lock);
    *ptr = msg;
    return size;
}