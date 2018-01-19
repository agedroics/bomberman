#include <game.h>

static uint8_t *field;
static int w;
static int h;

static uint8_t field_get(int x, int y) {
    return field[y * w + x];
}

static void field_set(int x, int y, uint8_t val) {
    field[y * w + x] = val;
}

static void init_field(int width, int height) {
    w = width;
    h = height;

    // make sure field size is odd
    if (w % 2 == 0) {
        ++w;
    }
    if (h % 2 == 0) {
        ++h;
    }

    field = calloc((size_t) w * h, 1);

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

/*
 * CHANCES
 *
 * power: 10%
 * speed: 10%
 * remote control: 5%
 * dynamite count: 10%
 * kick: 5%
 */
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

static void spawn_flames(millis_t cur_time, player_t *owner, uint8_t power, uint8_t x, uint8_t y, uint8_t direction) {
    if (!power || field_get(x, y) == BLOCK_WALL) {
        return;
    }
    int i = flame_create(cur_time, owner, x, y);
    if (field_get(x, y) == BLOCK_BOX) {
        flames[i].spawn_pwrup_type = random_pwrup();
        field_set(x, y, BLOCK_EMPTY);
        map_upd_create(x, y, BLOCK_EMPTY);
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

static void explode_dyn(int i, millis_t cur_time) {
    if (!(dynamites[i].owner->active_pwrups & ACTIVE_PWRUP_REMOTE) || dynamites[i].remote_detonated) {
        ++dynamites[i].owner->count;
    }
    uint8_t x = (uint8_t) dynamites[i].x;
    uint8_t y = (uint8_t) dynamites[i].y;
    flame_create(cur_time, dynamites[i].owner, x, y);
    spawn_flames(cur_time, dynamites[i].owner, dynamites[i].power, x - (uint8_t) 1, y, DIRECTION_LEFT);
    spawn_flames(cur_time, dynamites[i].owner, dynamites[i].power, x, y - (uint8_t) 1, DIRECTION_UP);
    spawn_flames(cur_time, dynamites[i].owner, dynamites[i].power, x + (uint8_t) 1, y, DIRECTION_RIGHT);
    spawn_flames(cur_time, dynamites[i].owner, dynamites[i].power, x, y + (uint8_t) 1, DIRECTION_DOWN);
}

static millis_t last_fill;
static int fill_x = 1;
static int fill_y = 1;
static uint8_t fill_direction = DIRECTION_RIGHT;

int do_tick(uint16_t timer, millis_t cur_time) {
    int i;
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

        for (i = 0; i < dyn_cnt; ++i) {
            if (dynamites[i].x - 1.5 < fill_x && dynamites[i].x + .5 > fill_x
                && dynamites[i].y - 1.5 < fill_y && dynamites[i].y + .5 > fill_y) {

                if (!(dynamites[i].owner->active_pwrups & ACTIVE_PWRUP_REMOTE) || dynamites[i].remote_detonated) {
                    ++dynamites[i].owner->count;
                }
                dyn_destroy(i--);
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

    for (i = 0; i < dyn_cnt; ++i) {
        if ((cur_time - dynamites[i].created >= DYNAMITE_TIMER * 1000 && !dynamites[i].remote_detonated)
            || (dynamites[i].remote_detonated && dynamites[i].owner->detonate_pressed)) {

            explode_dyn(i, cur_time);
            dyn_destroy(i--);
        } else if (dynamites[i].carrier) {
            if (dynamites[i].carrier->plant_pressed || dynamites[i].carrier->dead) {
                dynamites[i].x = (int) dynamites[i].carrier->x + .5;
                dynamites[i].y = (int) dynamites[i].carrier->y + .5;
                dynamites[i].carrier->carrying_dyn = 0;
                dynamites[i].carrier = NULL;
            } else {
                dynamites[i].x = dynamites[i].carrier->x;
                dynamites[i].y = dynamites[i].carrier->y;
            }
        } else if (dynamites[i].remote_detonated && dynamites[i].owner->dead) {
            if (dynamites[i].carrier) {
                dynamites[i].carrier->carrying_dyn = 0;
            }
            dyn_destroy(i--);
        } else {
            if (dynamites[i].is_sliding) {
                if (dynamites[i].slide_direction == DIRECTION_LEFT) {
                    int x = (int) dynamites[i].x;
                    dynamites[i].x -= (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get(x - 1, (int) dynamites[i].y) != BLOCK_EMPTY && dynamites[i].x - .5 <= x) {
                        dynamites[i].x = x + .5;
                        dynamites[i].is_sliding = 0;
                    }
                } else if (dynamites[i].slide_direction == DIRECTION_UP) {
                    int y = (int) dynamites[i].y;
                    dynamites[i].y -= (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get((int) dynamites[i].x, y - 1) != BLOCK_EMPTY && dynamites[i].y - .5 <= y) {
                        dynamites[i].y = y + .5;
                        dynamites[i].is_sliding = 0;
                    }
                } else if (dynamites[i].slide_direction == DIRECTION_RIGHT) {
                    int x = (int) dynamites[i].x + 1;
                    dynamites[i].x += (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get(x, (int) dynamites[i].y) != BLOCK_EMPTY && dynamites[i].x + .5 >= x) {
                        dynamites[i].x = x - .5;
                        dynamites[i].is_sliding = 0;
                    }
                } else if (dynamites[i].slide_direction == DIRECTION_DOWN) {
                    int y = (int) dynamites[i].y + 1;
                    dynamites[i].y += (double) DYNAMITE_SLIDE_V / TICK_RATE;
                    if (field_get((int) dynamites[i].x, y) != BLOCK_EMPTY && dynamites[i].y + .5 >= y) {
                        dynamites[i].y = y - .5;
                        dynamites[i].is_sliding = 0;
                    }
                }
            }
            player_t *it;
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, dynamites[i].x - .5, dynamites[i].y - .5)) {
                    if (it->active_pwrups & ACTIVE_PWRUP_KICK && dynamites[i].last_touched_by != it) {
                        dynamites[i].last_touched_by = it;
                        if (it->input & INPUT_LEFT
                            && (!dynamites[i].is_sliding || dynamites[i].slide_direction == DIRECTION_RIGHT)) {

                            dynamites[i].is_sliding = 1;
                            dynamites[i].slide_direction = DIRECTION_LEFT;
                        } else if (it->input & INPUT_UP
                                   && (!dynamites[i].is_sliding || dynamites[i].slide_direction == DIRECTION_DOWN)) {

                            dynamites[i].is_sliding = 1;
                            dynamites[i].slide_direction = DIRECTION_UP;
                        } else if (it->input & INPUT_RIGHT
                                   && (!dynamites[i].is_sliding || dynamites[i].slide_direction == DIRECTION_LEFT)) {
                            dynamites[i].is_sliding = 1;
                            dynamites[i].slide_direction = DIRECTION_RIGHT;
                        } else if (it->input & INPUT_DOWN
                                   && (!dynamites[i].is_sliding || dynamites[i].slide_direction == DIRECTION_UP)) {
                            dynamites[i].is_sliding = 1;
                            dynamites[i].slide_direction = DIRECTION_DOWN;
                        }
                    }
                    if (it->pick_up_pressed && !it->carrying_dyn) {
                        dynamites[i].is_sliding = 0;
                        dynamites[i].carrier = it;
                        it->carrying_dyn = 1;
                    }
                } else if (it == dynamites[i].last_touched_by) {
                    dynamites[i].last_touched_by = NULL;
                }
            }
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

        uint16_t directional_input = it->input & (uint16_t) (INPUT_LEFT | INPUT_UP | INPUT_RIGHT | INPUT_DOWN);
        if (directional_input == (INPUT_DOWN | INPUT_LEFT | INPUT_UP)
            || directional_input == (INPUT_UP | INPUT_RIGHT | INPUT_DOWN)) {

            it->input &= ~(INPUT_DOWN | INPUT_UP);
        } else if (directional_input == (INPUT_LEFT | INPUT_UP | INPUT_RIGHT)
                   || directional_input == (INPUT_RIGHT | INPUT_DOWN | INPUT_LEFT)) {

            it->input &= ~(INPUT_LEFT | INPUT_RIGHT);
        } else if ((directional_input & INPUT_LEFT && directional_input & INPUT_RIGHT)
                   || (directional_input & INPUT_UP && directional_input & INPUT_DOWN)) {

            it->input &= ~(INPUT_LEFT | INPUT_UP | INPUT_RIGHT | INPUT_DOWN);
        }

        double move_distance = (double) it->speed / TICK_RATE;

        int x = (int) it->x;
        int y = (int) it->y;

        int left = it->input & INPUT_LEFT;
        int up = it->input & INPUT_UP;
        int right = it->input & INPUT_RIGHT;
        int down = it->input & INPUT_DOWN;

        if ((left && it->x - move_distance < x + .5 &&
             ((it->y < y + .5 && field_get(x - 1, y) != BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY)
              || (it->y == y + .5 && field_get(x - 1, y) != BLOCK_EMPTY)
              || ((it->y > y + .5 && field_get(x - 1, y) != BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY))))
            || (right && it->x + move_distance > x + .5 &&
                ((it->y < y + .5 && field_get(x + 1, y) != BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY)
                 || (it->y == y + .5 && field_get(x + 1, y) != BLOCK_EMPTY)
                 || ((it->y > y + .5 && field_get(x + 1, y) != BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY))))
            || (up &&
                ((it->x > x + .5 && field_get(x, y - 1) == BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY && it->x - move_distance < x + .5)
                 || (it->x < x + .5 && field_get(x, y - 1) == BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY && it->x + move_distance > x + .5)))
            || (down &&
                ((it->x > x + .5 && field_get(x, y + 1) == BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY && it->x - move_distance < x + .5)
                 || (it->x < x + .5 && field_get(x, y + 1) == BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY && it->x + move_distance > x + .5)))) {

            it->x = x + .5;

        } else if (!right && ((left && it->y == y + .5 && (it->x < x + .5 || field_get(x - 1, y) == BLOCK_EMPTY || it->x - move_distance >= x + .5))
                   || (up &&
                ((it->x < x + .5 && field_get(x, y - 1) != BLOCK_EMPTY && field_get(x - 1, y - 1) == BLOCK_EMPTY)
                 || (it->x > x + .5 && field_get(x, y - 1) == BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY && it->x - move_distance >= x + .5)))
                   || (down &&
                ((it->x < x + .5 && field_get(x, y + 1) != BLOCK_EMPTY && field_get(x - 1, y + 1) == BLOCK_EMPTY)
                 || (it->x > x + .5 && field_get(x, y + 1) == BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY && it->x - move_distance >= x + .5))))) {

            it->direction = DIRECTION_LEFT;
            it->x -= move_distance;

        } else if (!left && ((right && it->y == y + .5 && (it->x < x + .5 || field_get(x + 1, y) == BLOCK_EMPTY || it->x + move_distance <= x + .5))
                   || (up &&
                       ((it->x > x + .5 && field_get(x, y - 1) != BLOCK_EMPTY && field_get(x + 1, y - 1) == BLOCK_EMPTY)
                        || (it->x < x + .5 && field_get(x, y - 1) == BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY && it->x + move_distance <= x + .5)))
                   || (down &&
                       ((it->x > x + .5 && field_get(x, y + 1) != BLOCK_EMPTY && field_get(x + 1, y + 1) == BLOCK_EMPTY)
                        || (it->x < x + .5 && field_get(x, y + 1) == BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY && it->x + move_distance <= x + .5))))) {

            it->direction = DIRECTION_RIGHT;
            it->x += move_distance;

        }

        if ((up && it->y - move_distance < y + .5 &&
                    ((it->x < x + .5 && field_get(x, y - 1) != BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY)
                     || (it->x == x + .5 && field_get(x, y - 1) != BLOCK_EMPTY)
                     || ((it->x > x + .5 && field_get(x, y - 1) != BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY))))
                   || (down && it->y + move_distance > y + .5 &&
                       ((it->x < x + .5 && field_get(x, y + 1) != BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY)
                        || (it->x == x + .5 && field_get(x, y + 1) != BLOCK_EMPTY)
                        || ((it->x > x + .5 && field_get(x, y + 1) != BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY))))
                   || (left &&
                       ((it->y > y + .5 && field_get(x - 1, y) == BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY && it->y - move_distance < y + .5)
                        || (it->y < y + .5 && field_get(x - 1, y) == BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY && it->y + move_distance > y + .5)))
                   || (right &&
                       ((it->y > y + .5 && field_get(x + 1, y) == BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY && it->y - move_distance < y + .5)
                        || (it->y < y + .5 && field_get(x + 1, y) == BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY && it->y + move_distance > y + .5)))) {

            it->y = y + .5;

        } else if (!down && ((up && it->x == x + .5 && (it->y < y + .5 || field_get(x, y - 1) == BLOCK_EMPTY || it->y - move_distance >= y + .5))
                   || (left &&
                       ((it->y < y + .5 && field_get(x - 1, y) != BLOCK_EMPTY && field_get(x - 1, y - 1) == BLOCK_EMPTY)
                        || (it->y > y + .5 && field_get(x - 1, y) == BLOCK_EMPTY && field_get(x - 1, y + 1) != BLOCK_EMPTY && it->y - move_distance >= y + .5)))
                   || (right &&
                       ((it->y < y + .5 && field_get(x + 1, y) != BLOCK_EMPTY && field_get(x + 1, y - 1) == BLOCK_EMPTY)
                        || (it->y > y + .5 && field_get(x + 1, y) == BLOCK_EMPTY && field_get(x + 1, y + 1) != BLOCK_EMPTY && it->y - move_distance >= y + .5))))) {

            it->direction = DIRECTION_UP;
            it->y -= move_distance;

        } else if (!up && ((down && it->x == x + .5 && (it->y < y + .5 || field_get(x, y + 1) == BLOCK_EMPTY || it->y + move_distance <= y + .5))
                   || (left &&
                       ((it->y > y + .5 && field_get(x - 1, y) != BLOCK_EMPTY && field_get(x - 1, y + 1) == BLOCK_EMPTY)
                        || (it->y < y + .5 && field_get(x - 1, y) == BLOCK_EMPTY && field_get(x - 1, y - 1) != BLOCK_EMPTY && it->y + move_distance <= y + .5)))
                   || (right &&
                       ((it->y > y + .5 && field_get(x + 1, y) != BLOCK_EMPTY && field_get(x + 1, y + 1) == BLOCK_EMPTY)
                        || (it->y < y + .5 && field_get(x + 1, y) == BLOCK_EMPTY && field_get(x + 1, y - 1) != BLOCK_EMPTY && it->y + move_distance <= y + .5))))) {

            it->direction = DIRECTION_DOWN;
            it->y += move_distance;

        }

        it->plant_pressed = 0;
        it->detonate_pressed = 0;
        it->pick_up_pressed = 0;
    }

    if (alive_players < 2) {
        return 1;
    }

    for (i = 0; i < flame_cnt; ++i) {
        if (cur_time - flames[i].created >= FLAME_TIMEOUT * 1000) {
            if (flames[i].spawn_pwrup_type != UINT8_MAX) {
                pwrup_create(cur_time, flames[i].x, flames[i].y, flames[i].spawn_pwrup_type);
            }
            flame_destroy(i--);
        } else if (field_get(flames[i].x, flames[i].y) == BLOCK_WALL) {
            flame_destroy(i--);
        } else {
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, flames[i].x, flames[i].y)) {
                    it->dead = 1;
                    if (it != flames[i].owner) {
                        ++flames[i].owner->frags;
                        printf("%s was killed by %s\n", it->name, flames[i].owner->name);
                    } else {
                        printf("%s committed suicide\n", it->name);
                    }
                }
            }
            int j;
            for (j = 0; j < dyn_cnt; ++j) {
                if (dynamites[j].x - 1.5 < flames[i].x && dynamites[j].x + .5 > flames[i].x
                    && dynamites[j].y - 1.5 < flames[i].y && dynamites[j].y + .5 > flames[i].y) {

                    explode_dyn(j, cur_time);
                    dyn_destroy(j--);
                }
            }
            for (j = 0; j < pwrup_cnt; ++j) {
                if (flames[i].x == pwrups[j].x && flames[i].y == pwrups[j].y) {
                    pwrup_destroy(j--);
                }
            }
        }
    }

    for (i = 0; i < pwrup_cnt; ++i) {
        if (cur_time - pwrups[i].created >= PWRUP_TIMEOUT * 1000 || field_get(pwrups[i].x, pwrups[i].y) == BLOCK_WALL) {
            pwrup_destroy(i--);
        } else {
            for (it = players; it; it = it->next) {
                if (!it->dead && player_intersects(it, pwrups[i].x, pwrups[i].y)) {
                    switch (pwrups[i].type) {
                        case PWRUP_POWER:
                            if (it->power < MAX_POWER) {
                                ++it->power;
                            }
                            break;
                        case PWRUP_SPEED:
                            if (it->speed < MAX_SPEED) {
                                it->speed += SPEED_INC_STEP;
                            }
                            break;
                        case PWRUP_REMOTE:
                            if (!(it->active_pwrups & ACTIVE_PWRUP_REMOTE)) {
                                it->active_pwrups |= ACTIVE_PWRUP_REMOTE;
                                it->max_count = 1;
                                it->count = 1;
                            }
                            break;
                        case PWRUP_COUNT:
                            if (it->max_count < MAX_COUNT && !(it->active_pwrups & ACTIVE_PWRUP_REMOTE)) {
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
                    pwrup_destroy(i--);
                    break;
                }
            }
        }
    }

    send_objects(timer);
    if (map_upd_cnt) {
        send_map_update();
    }

    return 0;
}

void setup_game(void) {
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
    init_field(FIELD_WIDTH, FIELD_HEIGHT);
    send_game_start(field, (uint8_t) w, (uint8_t) h);
}
