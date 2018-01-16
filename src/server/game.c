#include "game.h"

static uint8_t *field;
static int w;
static int h;

static uint8_t field_get(int x, int y) {
    return field[y * h + x];
}

static void field_set(int x, int y, uint8_t val) {
    field[y * h + x] = val;
}

static void init_field(int width, int height) {
    w = width;
    h = height;

    // minimum field size n+2 x n+2
    if (w < player_count + 2) {
        w = player_count + (uint8_t) 2;
    }
    if (h < player_count + 2) {
        h = player_count + (uint8_t) 2;
    }

    // make sure field size is odd
    if (w % 2 == 0) {
        ++w;
    }
    if (h % 2 == 0) {
        ++h;
    }

    field = malloc((size_t) w * h);

    // set field top and bottom walls
    uint8_t x, y;
    for (x = 0; x < w; ++x) {
        field_set(x, 0, BLOCK_WALL);
        field_set(x, h - 1, BLOCK_WALL);
    }

    for (y = 1; y < h - 1; ++y) {
        // set leftmost and rightmost walls
        field_set(0, y, BLOCK_WALL);
        field_set(w - 1, y, BLOCK_WALL);

        for (x = 1; x < w - 1; ++x) {
            if (x % 2 == 0 && y % 2 == 0) {
                field_set(x, y, BLOCK_WALL);
            } else if (rand() % 2 == 0) {
                field_set(x, y, BLOCK_BOX);
            }
        }
    }

    player_t *it;
    for (it = players; it; it = it->next) {
        do {
            x = (uint8_t) (rand() % (w / 2) * 2 + 1);
            y = (uint8_t) (rand() % (h / 2) * 2 + 1);
        } while (are_players_nearby(x, y, 2));
        it->x = x + .5;
        it->y = y + .5;

        // clear cross-shaped area around player
        field_set(x, y, BLOCK_EMPTY);
        if (x > 1) {
            field_set(x - 1, y, BLOCK_EMPTY);
        }
        if (y > 1) {
            field_set(x, y - 1, BLOCK_EMPTY);
        }
        if (x < w - 2) {
            field_set(x + 1, y, BLOCK_EMPTY);
        }
        if (y < h - 2) {
            field_set(x, y + 1, BLOCK_EMPTY);
        }
    }
}

void setup_game(void) {
    if (!field) {
        init_field(FIELD_WIDTH, FIELD_HEIGHT);
    }
    send_game_start(field, (uint8_t) w, (uint8_t) h);
}

static uint8_t random_pwrup(void) {
    int rnd = rand() % 20;
    if (rnd <= 1) {
        return PWRUP_POWER;
    } else if (rnd <= 3) {
        return PWRUP_SPEED;
    } else if (rnd == 4) {
        return PWRUP_REMOTE;
    } else if (rnd <= 6) {
        return PWRUP_COUNT;
    } else if (rnd == 7) {
        return PWRUP_KICK;
    } else {
        return UINT8_MAX;
    }
}

static void spawn_flames(time_t cur_time, player_t *owner, uint8_t power, uint8_t x, uint8_t y, uint8_t direction) {
    if (!power || field_get(x, y) == BLOCK_WALL) {
        return;
    }
    flame_create(cur_time, owner, x, y);
    if (field_get(x, y) == BLOCK_BOX) {
        map_upd_create(x, y, BLOCK_EMPTY);
        flames->spawn_pwrup_type = random_pwrup();
    } else {
        if (direction == DIRECTION_LEFT) {
            --x;
        } else if (direction == DIRECTION_UP) {
            --y;
        } else if (direction == DIRECTION_RIGHT) {
            ++x;
        } else {
            ++y;
        }
        spawn_flames(cur_time, owner, power - (uint8_t) 1, x, y, direction);
    }
}

static time_t last_fill;
static int fill_x = 1;
static int fill_y = 1;
static uint8_t fill_direction = DIRECTION_RIGHT;

int do_tick(uint16_t timer, time_t cur_time) {
    if (!timer && (!last_fill || cur_time - last_fill >= 1000 / FILL_SPEED)) {
        last_fill = cur_time;
        field_set(fill_x, fill_y, BLOCK_WALL);
        map_upd_create((uint8_t) fill_x, (uint8_t) fill_y, BLOCK_WALL);

        player_t *it;
        for (it = players; it; it = it->next) {
            if (!it->dead && player_intersects(it, fill_x, fill_y)) {
                it->dead = 1;
                printf("%s was killed by the map\n", it->name);
            }
        }

        dyn_t *dyn;
        for (dyn = dynamites; dyn; dyn = dyn->next) {
            if (dyn->x - 1.5 < fill_x && dyn->x + .5 > fill_x && dyn->y - 1.5 < fill_y && dyn->y + .5 > fill_y) {
                if (!(dyn->owner->active_pwrups & ACTIVE_PWRUP_REMOTE) || dyn->remote_detonated) {
                    ++dyn->owner->count;
                }
                dyn = dyn_destroy(dyn);
                if (!dyn) {
                    break;
                }
            }
        }

        switch (fill_direction) {
            case DIRECTION_LEFT:
                if (fill_x > h - fill_y - 1) {
                    --fill_x;
                } else {
                    --fill_y;
                    fill_direction = DIRECTION_UP;
                }
                break;
            case DIRECTION_UP:
                if (fill_y - 1 > fill_x) {
                    --fill_y;
                } else {
                    ++fill_x;
                    fill_direction = DIRECTION_RIGHT;
                }
                break;
            case DIRECTION_RIGHT:
                if (fill_x + 1 < w - fill_y) {
                    ++fill_x;
                } else {
                    ++fill_y;
                    fill_direction = DIRECTION_DOWN;
                }
                break;
            case DIRECTION_DOWN:
                if (fill_y < h - (w - fill_x)) {
                    ++fill_y;
                } else {
                    --fill_x;
                    fill_direction = DIRECTION_LEFT;
                }
                break;
            default:
                break;
        }
    }

    dyn_t *dyn;
    for (dyn = dynamites; dyn; dyn = dyn->next) {
        if ((cur_time - dyn->created) / 1000 >= DYNAMITE_TIMER || (dyn->remote_detonated && dyn->owner->detonate_pressed) || dyn->hit_by_flame) {
            if (!(dyn->owner->active_pwrups & ACTIVE_PWRUP_REMOTE) || dyn->remote_detonated) {
                ++dyn->owner->count;
            }
            uint8_t x = (uint8_t) dyn->x;
            uint8_t y = (uint8_t) dyn->y;
            flame_create(cur_time, dyn->owner, x, y);
            spawn_flames(cur_time, dyn->owner, dyn->power, x - (uint8_t) 1, y, DIRECTION_LEFT);
            spawn_flames(cur_time, dyn->owner, dyn->power, x, y - (uint8_t) 1, DIRECTION_UP);
            spawn_flames(cur_time, dyn->owner, dyn->power, x + (uint8_t) 1, y, DIRECTION_RIGHT);
            spawn_flames(cur_time, dyn->owner, dyn->power, x, y + (uint8_t) 1, DIRECTION_DOWN);
            dyn = dyn_destroy(dyn);
        } else if (dyn->carrier) {
            if (dyn->carrier->plant_pressed || dyn->carrier->dead) {
                dyn->x = (int) dyn->carrier->x + .5;
                dyn->y = (int) dyn->carrier->y + .5;
                dyn->carrier->carrying_dyn = 0;
                dyn->carrier = NULL;
            } else {
                dyn->x = dyn->carrier->x;
                dyn->y = dyn->carrier->y;
            }
        } else if (dyn->remote_detonated && dyn->owner->dead) {
            if (dyn->carrier) {
                dyn->carrier->carrying_dyn = 0;
            }
            dyn = dyn_destroy(dyn);
        } else {
            if (dyn->kicked_by) {
                if (dyn->slide_direction == DIRECTION_LEFT) {
                    int x = (int) dyn->x;
                    dyn->x -= (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get(x - 1, (int) dyn->y) != BLOCK_EMPTY && dyn->x - .5 <= x) {
                        dyn->x = x + .5;
                        dyn->kicked_by = NULL;
                    }
                } else if (dyn->slide_direction == DIRECTION_UP) {
                    int y = (int) dyn->y;
                    dyn->y -= (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get((int) dyn->x, y - 1) != BLOCK_EMPTY && dyn->y - .5 <= y) {
                        dyn->y = y + .5;
                        dyn->kicked_by = NULL;
                    }
                } else if (dyn->slide_direction == DIRECTION_RIGHT) {
                    int x = (int) dyn->x + 1;
                    dyn->x += (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get(x, (int) dyn->y) != BLOCK_EMPTY && dyn->x + .5 >= x) {
                        dyn->x = x - .5;
                        dyn->kicked_by = NULL;
                    }
                } else if (dyn->slide_direction == DIRECTION_DOWN) {
                    int y = (int) dyn->y + 1;
                    dyn->y += (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get((int) dyn->x, y) != BLOCK_EMPTY && dyn->y + .5 >= y) {
                        dyn->y = y - .5;
                        dyn->kicked_by = NULL;
                    }
                }
            }
            player_t *it;
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, dyn->x, dyn->y)) {
                    if (it->active_pwrups & ACTIVE_PWRUP_KICK && dyn->kicked_by != it) {
                        if (it->input & INPUT_LEFT) {
                            dyn->kicked_by = it;
                            dyn->slide_direction = DIRECTION_LEFT;
                        } else if (it->input & INPUT_UP) {
                            dyn->kicked_by = it;
                            dyn->slide_direction = DIRECTION_UP;
                        } else if (it->input & INPUT_RIGHT) {
                            dyn->kicked_by = it;
                            dyn->slide_direction = DIRECTION_RIGHT;
                        } else if (it->input & INPUT_DOWN) {
                            dyn->kicked_by = it;
                            dyn->slide_direction = DIRECTION_DOWN;
                        }
                    } else if (it->pick_up_pressed && !it->carrying_dyn) {
                        dyn->kicked_by = NULL;
                        dyn->carrier = it;
                        it->carrying_dyn = 1;
                    }
                } else if (it == dyn->kicked_by) {
                    dyn->kicked_by = NULL;
                }
            }
        }
        if (!dyn) {
            break;
        }
    }

    int alive_players = 0;
    player_t *it;
    for (it = players; it; it = it->next) {
        if (it->dead) {
            continue;
        }
        ++alive_players;

        if (it->plant_pressed && it->count && !it->carrying_dyn) {
            dyn_create(cur_time, it);
            --it->count;
        }

        double actual_speed = it->speed;
        uint16_t directional_input = it->input & (uint16_t) (INPUT_LEFT | INPUT_UP | INPUT_RIGHT | INPUT_DOWN);
        if (directional_input == (INPUT_DOWN | INPUT_LEFT | INPUT_UP) || directional_input == (INPUT_UP | INPUT_RIGHT | INPUT_DOWN)) {
            it->input &= ~(INPUT_DOWN | INPUT_UP);
        } else if (directional_input == (INPUT_LEFT | INPUT_UP | INPUT_RIGHT) || directional_input == (INPUT_RIGHT | INPUT_DOWN | INPUT_LEFT)) {
            it->input &= ~(INPUT_LEFT | INPUT_RIGHT);
        } else if (directional_input == (INPUT_LEFT | INPUT_UP) || directional_input == (INPUT_UP | INPUT_RIGHT)
            || directional_input == (INPUT_RIGHT | INPUT_DOWN) || directional_input == (INPUT_DOWN | INPUT_LEFT)) {
            actual_speed *= sqrt(2) / 2;
        } else if ((directional_input & (INPUT_LEFT | INPUT_RIGHT)) || (directional_input & (INPUT_UP | INPUT_DOWN))) {
            it->input &= ~(INPUT_LEFT | INPUT_UP | INPUT_RIGHT | INPUT_DOWN);
        }

        if (it->input & INPUT_LEFT) {
            it->direction = DIRECTION_LEFT;
            int y = (int) it->y;
            int x = (int) it->x - 1;
            it->x -= actual_speed / TICK_RATE;
            if ((field_get(x, y - 1) != BLOCK_EMPTY && player_intersects(it, x, y - 1))
                || (field_get(x, y) != BLOCK_EMPTY && player_intersects(it, x, y))
                || (field_get(x, y + 1) != BLOCK_EMPTY && player_intersects(it, x, y + 1))) {
                it->x = x + 1 + (double) PLAYER_SIZE / 2;
            }
        }
        if (it->input & INPUT_UP) {
            it->direction = DIRECTION_UP;
            int x = (int) it->x;
            int y = (int) it->y - 1;
            it->y -= actual_speed / TICK_RATE;
            if ((field_get(x - 1, y) != BLOCK_EMPTY && player_intersects(it, x - 1, y))
                || (field_get(x, y) != BLOCK_EMPTY && player_intersects(it, x, y))
                || (field_get(x + 1, y) != BLOCK_EMPTY && player_intersects(it, x + 1, y))) {
                it->y = y + 1 + (double) PLAYER_SIZE / 2;
            }
        }
        if (it->input & INPUT_RIGHT) {
            it->direction = DIRECTION_RIGHT;
            int y = (int) it->y;
            int x = (int) it->x + 1;
            it->x += actual_speed / TICK_RATE;
            if ((field_get(x, y - 1) != BLOCK_EMPTY && player_intersects(it, x, y - 1))
                || (field_get(x, y) != BLOCK_EMPTY && player_intersects(it, x, y))
                || (field_get(x, y + 1) != BLOCK_EMPTY && player_intersects(it, x, y + 1))) {
                it->x = x - (double) PLAYER_SIZE / 2;
            }
        }
        if (it->input & INPUT_DOWN) {
            it->direction = DIRECTION_DOWN;
            int x = (int) it->x;
            int y = (int) it->y + 1;
            it->y += actual_speed / TICK_RATE;
            if ((field_get(x - 1, y) != BLOCK_EMPTY && player_intersects(it, x - 1, y))
                || (field_get(x, y) != BLOCK_EMPTY && player_intersects(it, x, y))
                || (field_get(x + 1, y) != BLOCK_EMPTY && player_intersects(it, x + 1, y))) {
                it->y = y - (double) PLAYER_SIZE / 2;
            }
        }

        it->plant_pressed = 0;
        it->detonate_pressed = 0;
        it->pick_up_pressed = 0;
    }

    if (alive_players < 2) {
        return 1;
    }

    flame_t *flame;
    for (flame = flames; flame; flame = flame->next) {
        if ((cur_time - flame->created) / 1000 >= FLAME_TIMEOUT) {
            if (flame->spawn_pwrup_type != UINT8_MAX) {
                pwrup_create(cur_time, flame->x, flame->y, flame->spawn_pwrup_type);
            }
            flame = flame_destroy(flame);
        } else if (field_get(flame->x, flame->y) == BLOCK_WALL) {
            flame = flame_destroy(flame);
        } else {
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, flame->x, flame->y)) {
                    it->dead = 1;
                    if (it != flame->owner) {
                        ++flame->owner->frags;
                        printf("%s was killed by %s\n", it->name, flame->owner->name);
                    } else {
                        printf("%s committed suicide\n", it->name);
                    }
                }
            }
            for (dyn = dynamites; dyn; dyn = dyn->next) {
                if (dyn->x - 1.5 < flame->x && dyn->x + .5 > flame->x && dyn->y - 1.5 < flame->y && dyn->y + .5 > flame->y) {
                    dyn->hit_by_flame = 1;
                }
            }
            pwrup_t *pwrup;
            for (pwrup = pwrups; pwrup; pwrup = pwrup->next) {
                if (flame->x == pwrup->x && flame->y == pwrup->y) {
                    pwrup = pwrup_destroy(pwrup);
                    if (!pwrup) {
                        break;
                    }
                }
            }
        }
        if (!flame) {
            break;
        }
    }

    pwrup_t *pwrup;
    for (pwrup = pwrups; pwrup; pwrup = pwrup->next) {
        if ((cur_time - pwrup->created) / 1000 >= PWRUP_TIMEOUT || field_get(pwrup->x, pwrup->y) == BLOCK_WALL) {
            pwrup = pwrup_destroy(pwrup);
        } else {
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, pwrup->x, pwrup->y)) {
                    switch (pwrup->type) {
                        case PWRUP_POWER:
                            if (it->power < MAX_POWER) {
                                ++it->power;
                            }
                            break;
                        case PWRUP_SPEED:
                            if (it->speed < MAX_SPEED) {
                                ++it->speed;
                            }
                            break;
                        case PWRUP_REMOTE:
                            it->active_pwrups |= ACTIVE_PWRUP_REMOTE;
                            it->max_count = 1;
                            it->count = 1;
                            break;
                        case PWRUP_COUNT:
                            if (it->max_count < MAX_COUNT) {
                                ++it->max_count;
                                ++it->count;
                            }
                            break;
                        case PWRUP_KICK:
                            it->active_pwrups |= ACTIVE_PWRUP_KICK;
                            break;
                        default:
                            break;
                    }
                    pwrup = pwrup_destroy(pwrup);
                    break;
                }
            }
        }
        if (!pwrup) {
            break;
        }
    }

    send_objects(timer);
    if (map_upd_cnt) {
        send_map_update();
    }

    return 0;
}

void reset_game(void) {
    if (field) {
        free(field);
        field = NULL;
    }

    last_fill = 0;
    fill_x = 1;
    fill_y = 1;
    fill_direction = DIRECTION_RIGHT;

    cleanup_objects();

    player_t *it;
    for (it = players; it; it = it->next) {
        it->ready = 0;
        it->input = 0;
        it->x = 0;
        it->y = 0;
        it->frags = 0;
        it->power = DEFAULT_POWER;
        it->speed = DEFAULT_SPEED;
        it->count = DEFAULT_COUNT;
        it->max_count = DEFAULT_COUNT;
        it->active_pwrups = 0;
        it->dead = 0;
        it->plant_pressed = 0;
        it->detonate_pressed = 0;
        it->pick_up_pressed = 0;
        it->carrying_dyn = 0;
    }
}
