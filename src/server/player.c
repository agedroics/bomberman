#include <player.h>

static uint8_t max_id = 0;
pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
player_t *players = NULL;
uint8_t player_count = 0;

player_t *add_player(int fd, char *name) {
    if (player_count + 1 > MAX_PLAYERS) {
        return NULL;
    }
    player_t *player = calloc(1, sizeof(struct player));
    player->prev = NULL;
    player->next = players;
    if (players) {
        players->prev = player;
    }
    players = player;
    player->fd = fd;
    strncpy(player->name, name, 23);
    player->id = max_id++;
    player->power = DEFAULT_POWER;
    player->speed = DEFAULT_SPEED;
    player->count = DEFAULT_COUNT;
    player->max_count = DEFAULT_COUNT;
    ++player_count;
    return player;
}

void remove_player(player_t *player) {
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
    player_t *it;
    for (it = players; it; it = it->next) {
        if (!it->ready) {
            return 0;
        }
    }
    return 1;
}

void set_players_not_ready(void) {
    player_t *it;
    for (it = players; it; it = it->next) {
        it->ready = 0;
    }
}

int are_players_nearby(uint8_t x, uint8_t y, uint8_t distance) {
    int x1 = x - distance;
    int x2 = x + 1 + distance;
    int y1 = y - distance;
    int y2 = y + 1 + distance;
    player_t *it;
    for (it = players; it; it = it->next) {
        if (!it->x) {
            continue;
        }
        if (((int) it->x == x && it->y >= y1 && it->y <= y2)
            || ((int) it->y == y && it->x >= x1 && it->x <= x2)) {
            return 1;
        }
    }
    return 0;
}

int player_intersects(player_t *player, double x, double y) {
    double px1 = player->x - (double) PLAYER_SIZE / 2;
    double px2 = px1 + PLAYER_SIZE;
    double py1 = player->y - (double) PLAYER_SIZE / 2;
    double py2 = py1 + PLAYER_SIZE;
    return px1 < x + 0.99 && px2 > x + 0.01 && py1 < y + 0.99 && py2 > y + 0.01;
}
