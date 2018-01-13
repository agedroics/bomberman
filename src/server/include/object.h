#ifndef BOMBERMAN_OBJECT_H
#define BOMBERMAN_OBJECT_H

#include <math.h>
#include "player.h"
#include "../../common/include/protocol.h"

#define DYNAMITE_TIMER 3
#define DYNAMITE_SLIDE_V 6
#define FLAME_TIMEOUT 2
#define PWRUP_TIMEOUT 10

typedef struct dyn {
    time_t created;
    player *owner;
    player *carrier;
    double x;
    double y;
    uint8_t power;
    int remote_detonated;
    int owner_can_kick;
    player *kicked_by;
    uint8_t slide_direction;
    struct dyn *prev;
    struct dyn *next;
} dyn_t;

extern dyn_t *dynamites;
extern uint8_t dyn_cnt;

void dyn_create(time_t cur_time, player *owner);

dyn_t *dyn_destroy(dyn_t *dyn);

typedef struct flame {
    time_t created;
    player *owner;
    uint8_t x;
    uint8_t y;
    struct flame *prev;
    struct flame *next;
} flame_t;

extern flame_t *flames;
extern uint8_t flame_cnt;

void flame_create(time_t created, player *owner, uint8_t x, uint8_t y);

flame_t *flame_destroy(flame_t *flame);

typedef struct pwrup {
    time_t created;
    uint8_t x;
    uint8_t y;
    uint8_t type;
    struct pwrup *prev;
    struct pwrup *next;
} pwrup_t;

extern pwrup_t *pwrups;
extern uint8_t pwrup_cnt;

void pwrup_create(time_t cur_time, uint8_t x, uint8_t y, uint8_t type);

pwrup_t *pwrup_destroy(pwrup_t *pwrup);

typedef struct map_upd {
    uint8_t x;
    uint8_t y;
    uint8_t block;
    struct map_upd *next;
} map_upd_t;

extern map_upd_t *map_updates;
extern uint16_t map_upd_cnt;

void map_upd_create(uint8_t x, uint8_t y, uint8_t block);

void cleanup_objects(void);

#endif //BOMBERMAN_OBJECT_H
