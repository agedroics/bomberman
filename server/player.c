#include "player.h"

player *players     = NULL;
int    player_count = 0;

struct player {
    int  id;
    char name[24];
    int  ready;
    int  dead;
    int  input;
    int  direction;
    int  x;
    int  y;
    int  frags;
    int  power;
    int  speed;
    int  count;
    int  active_pwrups;
};

void create_player(player *player, char *name) {
    player->id            = player_count++;
    player->ready         = 0;
    player->dead          = 0;
    player->input         = 0;
    player->direction     = DIRECTION_UP;
    player->frags         = 0;
    player->power         = 1;
    player->speed         = 1;
    player->count         = 1;
    player->active_pwrups = 0;
    strcpy(player->name, name);
}

int add_player(char *name) {
    pthread_mutex_lock(&players_lock);
    if (!players) {
        players = malloc(max_players * sizeof(void *));
    }
    int id = player_count - 1;
    create_player(players + id, name);
    pthread_mutex_unlock(&players_lock);
    return id;
}

void clear_players() {
    pthread_mutex_lock(&players_lock);
    if (players) {
        free(players);
        players = NULL;
    }
    pthread_mutex_unlock(&players_lock);
}