#include <state.h>

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;

uint16_t timer;

uint8_t *field;
uint8_t field_width;
uint8_t field_height;

uint8_t field_get(int x, int y) {
    return field[y * field_width + x];
}

void field_set(int x, int y, uint8_t block_type) {
    field[y * field_width + x] = block_type;
}

player_info player_infos[256];
player_t players[256];
uint8_t player_cnt;

dyn_t dynamites[256];
uint8_t dyn_cnt;
uint8_t dyn_timer;

flame_t flames[256];
uint8_t flame_cnt;
int *flame_map;

int flame_map_get(int x, int y) {
    return flame_map[y * field_width + x];
}

void flame_map_set(int x, int y, int val) {
    flame_map[y * field_width + x] = val;
}

pwrup_t pwrups[256];
uint8_t pwrup_cnt;

uint8_t winner_ids[256];
uint8_t winner_cnt;

box_fade_t *box_fades;

void box_fade_create(uint8_t x, uint8_t y) {
    box_fade_t *box_fade = malloc(sizeof(box_fade_t));
    box_fade->x = x;
    box_fade->y = y;
    box_fade->keyframe_start = 0;
    box_fade->prev = NULL;
    box_fade->next = box_fades;
    if (box_fades) {
        box_fades->prev = box_fade;
    }
    box_fades = box_fade;
}

box_fade_t *box_fade_destroy(box_fade_t *box_fade) {
    box_fade_t *next = box_fade->next;
    if (next) {
        next->prev = box_fade->prev;
    }
    if (box_fade->prev) {
        box_fade->prev->next = next;
    }
    if (box_fade == box_fades) {
        box_fades = next;
    }
    free(box_fade);
    return next;
}
