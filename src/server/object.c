#include "object.h"

dyn_t dynamites[256];
uint8_t dyn_cnt;

void dyn_create(time_t cur_time, player_t *owner) {
    if (dyn_cnt == 256) {
        return;
    }
    dyn_t *dyn = dynamites + dyn_cnt;
    dyn->created = cur_time;
    dyn->owner = owner;
    dyn->carrier = NULL;
    dyn->x = (int) owner->x + .5;
    dyn->y = (int) owner->y + .5;
    dyn->power = owner->power;
    dyn->remote_detonated = owner->active_pwrups & ACTIVE_PWRUP_REMOTE;
    dyn->kicked_by = NULL;
    ++dyn_cnt;
}

void dyn_destroy(int i) {
    dynamites[i] = dynamites[dyn_cnt - 1];
    --dyn_cnt;
}

flame_t flames[256];
uint8_t flame_cnt;

int flame_create(time_t created, player_t *owner, uint8_t x, uint8_t y) {
    if (flame_cnt == 256) {
        return 255;
    }
    flame_t *flame = flames + flame_cnt;
    flame->created = created;
    flame->owner = owner;
    flame->x = x;
    flame->y = y;
    flame->spawn_pwrup_type = UINT8_MAX;
    return flame_cnt++;
}

void flame_destroy(int i) {
    flames[i] = flames[flame_cnt - 1];
    --flame_cnt;
}

pwrup_t pwrups[256];
uint8_t pwrup_cnt;

void pwrup_create(time_t cur_time, uint8_t x, uint8_t y, uint8_t type) {
    if (pwrup_cnt == 256) {
        return;
    }
    pwrup_t *pwrup = pwrups + pwrup_cnt;
    pwrup->created = cur_time;
    pwrup->x = x;
    pwrup->y = y;
    pwrup->type = type;
    ++pwrup_cnt;
}

void pwrup_destroy(int i) {
    pwrups[i] = pwrups[pwrup_cnt - 1];
    --pwrup_cnt;
}

map_upd_t *map_updates;
uint16_t map_upd_cnt;

void map_upd_create(uint8_t x, uint8_t y, uint8_t block) {
    map_upd_t *map_upd = malloc(sizeof(map_upd_t));
    map_upd->x = x;
    map_upd->y = y;
    map_upd->block = block;
    map_upd->next = map_updates;
    map_updates = map_upd;
    ++map_upd_cnt;
}

void cleanup_objects(void) {
    dyn_cnt = 0;
    flame_cnt = 0;
    pwrup_cnt = 0;

    map_upd_t *map_upd;
    while ((map_upd = map_updates)) {
        map_updates = map_upd->next;
        free(map_upd);
    }
    map_upd_cnt = 0;
}

void remove_player_objects(player_t *player) {
    int i;
    for (i = 0; i < dyn_cnt; ++i) {
        if (dynamites[i].owner == player) {
            dyn_destroy(i--);
        }
    }

    for (i = 0; i < flame_cnt; ++i) {
        if (flames[i].owner == player) {
            flame_destroy(i--);
        }
    }
}
