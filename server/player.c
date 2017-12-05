#include "player.h"

pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
player *players = NULL;
char *taken_slots = NULL;
int player_count = 0;
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

player *get_player(int id) {
    if (taken_slots[id]) {
        return players + id;
    }
    return NULL;
}

int add_player(char *name) {
    pthread_mutex_lock(&players_lock);
    if (!players) {
        players = malloc(max_players * sizeof(void *));
        taken_slots = calloc((size_t) max_players, 1);
    }
    int id;
    for (id = 0; taken_slots[id] && id < max_players; id++);
    if (id == max_players && taken_slots[id]) {
        id = -1;
    } else {
        create_player(id, name);
        taken_slots[id] = 1;
        player_count++;
    }
    pthread_mutex_unlock(&players_lock);
    return id;
}

void remove_player(int id) {
    pthread_mutex_lock(&players_lock);
    taken_slots[id] = 0;
    player_count--;
    pthread_mutex_unlock(&players_lock);
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