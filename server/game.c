#include "game.h"

#define PLAYER_RADIUS 0.3
#define DYNAMITE_TIMER 2
#define DYNAMITE_SLIDE_V 5

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

static int are_players_nearby(uint8_t x, uint8_t y, uint8_t distance) {
    uint16_t x1 = (uint16_t) ((x - distance) * 10);
    uint16_t x2 = (uint16_t) ((x + 1 + distance) * 10);
    uint16_t y1 = (uint16_t) ((y - distance) * 10);
    uint16_t y2 = (uint16_t) ((y + 1 + distance) * 10);
    player *it;
    for (it = players; it; it = it->next) {
        if (!it->x) {
            continue;
        }
        if (it->x / 10 == x && it->y >= y1 && it->y <= y2
            || it->y / 10 == y && it->x >= x1 && it->x <= x2) {
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
        it->x = (uint16_t) (x * 10 + 5);
        it->y = (uint16_t) (y * 10 + 5);

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
    msg[offset++] = DYNAMITE_TIMER;

    // dynamite slide velocity in u/10s
    msg[offset] = DYNAMITE_SLIDE_V * 10;

    broadcast(msg, size);
    free(msg);
}

typedef struct {
    uint8_t id;
    uint8_t frags;
} score;

static int score_compar(const void *ptr1, const void *ptr2) {
    return ((score *) ptr1)->frags - ((score *) ptr2)->frags;
}

void send_game_over(void) {
    score *scores = malloc(player_count * sizeof(score));
    int i;
    player *it;
    for (i = 0, it = players; it; ++i, it = it->next) {
        scores[i].id = it->id;
        scores[i].frags = it->frags;
    }
    qsort(scores, player_count, sizeof(score), score_compar);
    size_t size = 2 + player_count;
    uint8_t *msg = malloc(size);
    msg[0] = GAME_OVER;
    msg[1] = player_count;
    for (i = 0; i < player_count; ++i) {
        msg[i + 2] = scores[i].id;
    }
    free(scores);
    broadcast(msg, size);
    free(msg);
}

static int player_intersects(player *player, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    double px = player->x / 10.;
    double py = player->y / 10.;
    double dx = px - MAX(x, MIN(px, x + width));
    double dy = py - MAX(y, MIN(py, y + height));
    return (dx * dx + dy * dy) < (PLAYER_RADIUS * PLAYER_RADIUS);
}

typedef struct object {
    time_t created;
    player *owner;
    player *holder; // only for dynamite
    uint8_t x;
    uint8_t y;
    uint8_t info; // power for dynamite, pwrup type for pwrup
    struct object *next;
    struct object *prev;
} object;

static object *dynamites;
static uint8_t dyn_cnt;

static object *flames;
static uint8_t flame_cnt;

static object *pwrups;
static uint8_t pwrup_cnt;

void do_tick(uint16_t timer) {
    object *obj_it;
    player *it;
    size_t max_size = (size_t) 7 + dyn_cnt * 2 + flame_cnt * 2 + pwrup_cnt * 3 + player_count * 11;
    size_t size = 0;
    uint8_t *msg = malloc(max_size);
    msg[size++] = OBJECTS;
    memcpy(msg + size, &timer, 2);
    size += 2;
    msg[size++] = dyn_cnt;
    for (obj_it = dynamites; obj_it; obj_it = obj_it->next) {
        msg[size++] = obj_it->x;
        msg[size++] = obj_it->y;
    }
    msg[size++] = flame_cnt;
    for (obj_it = flames; obj_it; obj_it = obj_it->next) {
        msg[size++] = obj_it->x;
        msg[size++] = obj_it->y;
    }
    msg[size++] = pwrup_cnt;
    for (obj_it = pwrups; obj_it; obj_it = obj_it->next) {
        msg[size++] = obj_it->x;
        msg[size++] = obj_it->y;
        msg[size++] = obj_it->info;
    }
    msg[size++] = player_count;
    for (it = players; it; it = it->next) {

        // TODO: collision checks
        if (it->input & INPUT_UP) {
            it->direction = DIRECTION_UP;
            it->y -= (uint16_t) round(it->speed / TICK_RATE * 10);
        } else if (it->input & INPUT_LEFT) {
            it->direction = DIRECTION_LEFT;
            it->x -= (uint16_t) round(it->speed / TICK_RATE * 10);
        } else if (it->input & INPUT_RIGHT) {
            it->direction = DIRECTION_RIGHT;
            it->x += (uint16_t) round(it->speed / TICK_RATE * 10);
        } else if (it->input & INPUT_DOWN) {
            it->direction = DIRECTION_DOWN;
            it->y += (uint16_t) round(it->speed / TICK_RATE * 10);
        }

        msg[size++] = it->id;
        msg[size++] = it->dead;
        if (!it->dead) {
            memcpy(msg + size, &it->x, 2);
            memcpy(msg + size + 2, &it->y, 2);
            size += 4;
            msg[size++] = it->direction;
            msg[size++] = it->active_pwrups;
            msg[size++] = it->power;
            msg[size++] = it->speed;
            msg[size++] = it->count;
        }
    }
    broadcast(msg, size);
    free(msg);
}

void reset_game(void) {
    if (field) {
        free(field);
        field = NULL;
    }

    dyn_cnt = 0;
    if (dynamites) {
        free(dynamites);
        dynamites = NULL;
    }

    flame_cnt = 0;
    if (flames) {
        free(flames);
        flames = NULL;
    }

    pwrup_cnt = 0;
    if (pwrups) {
        free(pwrups);
        pwrups = NULL;
    }

    player *it;
    for (it = players; it; it = it->next) {
        it->ready = 0;
        it->input = 0;
        it->x = 0;
        it->y = 0;
        it->frags = 0;
        it->power = 1;
        it->speed = 3;
        it->count = 1;
        it->active_pwrups = 0;
        it->dead = 0;
    }
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
    }
    players = player;
    player->fd = fd;
    strncpy(player->name, name, 23);
    player->id = max_id++;
    player->power = 1;
    player->speed = 3;
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
        send(it->fd, msg, size, MSG_DONTWAIT);
    }
}

void lock_players(void) {
    pthread_mutex_lock(&players_lock);
}

void unlock_players(void) {
    pthread_mutex_unlock(&players_lock);
}
