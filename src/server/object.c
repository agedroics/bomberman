#include "object.h"

dyn_t *dynamites;
uint8_t dyn_cnt;

void dyn_create(time_t cur_time, player *owner) {
    dyn_t *dyn = malloc(sizeof(dyn_t));
    dyn->created = cur_time;
    dyn->owner = owner;
    dyn->carrier = NULL;
    dyn->x = (int) owner->x + .5;
    dyn->y = (int) owner->y + .5;
    dyn->power = owner->power;
    dyn->remote_detonated = owner->active_pwrups & ACTIVE_PWRUP_REMOTE;
    dyn->kicked_by = NULL;
    dyn->prev = NULL;
    dyn->next = dynamites;
    dynamites = dyn;
    ++dyn_cnt;
}

dyn_t *dyn_destroy(dyn_t *dyn) {
    dyn_t *next = dyn->next;
    if (dyn->prev) {
        dyn->prev->next = next;
    }
    if (next) {
        next->prev = dyn->prev;
    }
    if (dynamites == dyn) {
        dynamites = next;
    }
    free(dyn);
    --dyn_cnt;
    return next;
}

flame_t *flames;
uint8_t flame_cnt;

void flame_create(time_t created, player *owner, uint8_t x, uint8_t y) {
    flame_t *flame = malloc(sizeof(flame_t));
    flame->created = created;
    flame->owner = owner;
    flame->x = x;
    flame->y = y;
    flame->prev = NULL;
    flame->next = flames;
    if (flames) {
        flames->prev = flame;
    }
    flames = flame;
    ++flame_cnt;
}

flame_t *flame_destroy(flame_t *flame) {
    flame_t *next = flame->next;
    if (flame->prev) {
        flame->prev->next = next;
    }
    if (next) {
        next->prev = flame->prev;
    }
    if (flames == flame) {
        flames = next;
    }
    free(flame);
    --flame_cnt;
    return next;
}

pwrup_t *pwrups;
uint8_t pwrup_cnt;

void pwrup_create(time_t cur_time, uint8_t x, uint8_t y, uint8_t type) {
    pwrup_t *pwrup = malloc(sizeof(pwrup_t));
    pwrup->created = cur_time;
    pwrup->x = x;
    pwrup->y = y;
    pwrup->type = type;
    pwrup->prev = NULL;
    pwrup->next = pwrups;
    if (pwrups) {
        pwrups->prev = pwrup;
    }
    pwrups = pwrup;
    ++pwrup_cnt;
}

pwrup_t *pwrup_destroy(pwrup_t *pwrup) {
    pwrup_t *next = pwrup->next;
    if (pwrup->prev) {
        pwrup->prev->next = next;
    }
    if (next) {
        next->prev = pwrup->prev;
    }
    if (pwrups == pwrup) {
        pwrups = next;
    }
    free(pwrup);
    --pwrup_cnt;
    return next;
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
    while (dynamites) {
        dyn_destroy(dynamites);
    }

    flame_cnt = 0;
    while (flames) {
        flame_destroy(flames);
    }

    pwrup_cnt = 0;
    while (pwrups) {
        pwrup_destroy(pwrups);
    }

    map_upd_cnt = 0;
    map_upd_t *map_upd;
    while ((map_upd = map_updates)) {
        map_updates = map_upd->next;
        free(map_upd);
    }
}
