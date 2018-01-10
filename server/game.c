#include "game.h"

static pthread_mutex_t players_lock = PTHREAD_MUTEX_INITIALIZER;
static player *players = NULL;
static uint8_t max_id = 0;
static uint8_t *field;
static uint8_t w = 15;
static uint8_t h = 15;
uint8_t player_count = 0;
uint8_t max_players = 8;

void send_lobby_status(void) {
    size_t size = 2 + 25 * (size_t) player_count;
    uint8_t *msg = calloc(size, 1);
    msg[0] = LOBBY_STATUS;
    msg[1] = player_count;
    int i;
    player *it;
    for (i = 0, it = players; i < player_count && it; ++i, it = it->next) {
        msg[2 + i * 25] = it->id;
        memcpy(msg + 3 + i * 25, it->name, 23);
        msg[(i + 1) * 25 + 1] = it->ready;
    }
    broadcast(msg, size);
    free(msg);
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

static uint8_t field_get(uint8_t x, uint8_t y) {
    return field[y * h + x];
}

static void field_set(uint8_t x, uint8_t y, uint8_t val) {
    field[y * h + x] = val;
}

static int are_players_nearby(uint8_t x, uint8_t y, uint8_t radius) {
    player *it;
    uint16_t x1 = x - radius;
    uint16_t x2 = x + radius;
    uint16_t y1 = y - radius;
    uint16_t y2 = y + radius;
    for (it = players; it; it = it->next) {
        if (!it->x) {
            continue;
        }
        if (it->x / 10 == x && it->y / 10 >= y1 && it->y / 10 <= y2
                || it->y / 10 == y && it->x / 10 >= x1 && it->x / 10 <= x2) {
            return 1;
        }
    }
    return 0;
}

static void init_field(void) {
    // minimum field size n+2 x n+2
    w >= player_count + 2 ?: (w = player_count + (uint8_t) 2);
    h >= player_count + 2 ?: (h = player_count + (uint8_t) 2);

    // make sure field size is odd
    w % 2 ?: ++w;
    h % 2 ?: ++h;

    field = malloc(w * h);

    // set field top and bottom walls
    uint8_t x, y;
    for (x = 0; x < w; ++x) {
        field_set(x, 0, BLOCK_WALL);
        field_set(x, h - (uint8_t) 1u, BLOCK_WALL);
    }

    for (y = 1; y < h - 1; ++y) {
        // set leftmost and rightmost walls
        field_set(0, y, BLOCK_WALL);
        field_set(w - (uint8_t) 1u, y, BLOCK_WALL);

        for (x = 1; x < w - 1; ++x) {
            if (x % 2 == 0 && y % 2 == 0) {
                field_set(x, y, BLOCK_WALL);
            } else if (rand() % 2 == 0) {
                field_set(x, y, BLOCK_BOX);
            }
        }
    }

    player *it;
    for (it = players; it; it = it->next) {
        do {
            x = (uint8_t) ((rand() * 2) % (w - 2) + 1);
            y = (uint8_t) ((rand() * 2) % (h - 2) + 1);
        } while (are_players_nearby(x, y, 2));
        it->x = (uint16_t) (x * 10);
        it->y = (uint16_t) (y * 10);

        // clear cross-shaped area around player
        if (x > 1) {
            field_set(x - (uint8_t) 1, y, BLOCK_EMPTY);
        }
        if (y > 1) {
            field_set(x, y - (uint8_t) 1, BLOCK_EMPTY);
        }
        if (x < w - 2) {
            field_set(x + (uint8_t) 1, y, BLOCK_EMPTY);
        }
        if (y < h - 2) {
            field_set(x, y + (uint8_t) 1, BLOCK_EMPTY);
        }
    }
}

void send_game_start(void) {
    if (field) {
        free(field);
    }
    init_field();
    size_t size = 2 + 29 * (size_t) player_count + 2 + w * h + 2;
    uint8_t *msg = calloc(size, 1);
    msg[0] = GAME_START;
    msg[1] = player_count;
    int i;
    player *it;
    for (i = 0, it = players; it; ++i, it = it->next) {
        msg[2 + i * 29] = it->id;
        memcpy(2 + msg + i * 29 + 1, it->name, 23);
        memcpy(2 + msg + i * 29 + 24, &it->x, 2);
        memcpy(2 + msg + i * 29 + 26, &it->y, 2);
        memcpy(2 + msg + i * 29 + 28, &it->direction, 1);
    }
    size_t offset = (size_t) (2 + 29 * player_count);
    msg[offset++] = w;
    msg[offset++] = h;
    memcpy(msg + offset, field, w * h);
    offset += w * h;

    // dynamite timer in s
    msg[offset++] = 2;

    // dynamite slide velocity in u/10s
    msg[offset] = 30;

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

void broadcast(void *msg, size_t size) {
    player *it;
    for (it = players; it; it = it->next) {
        // maybe MSG_DONTWAIT
        write(it->fd, msg, size);
    }
}

void lock_players(void) {
    pthread_mutex_lock(&players_lock);
}

void unlock_players(void) {
    pthread_mutex_unlock(&players_lock);
}