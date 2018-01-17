#include "packet.h"

int send_join_request(int fd, char *nickname) {
    char msg[24];
    msg[0] = JOIN_REQUEST;
    strncpy(msg + 1, nickname, 23);
    return write(fd, msg, 24) == -1 ? -1 : 0;
}

int send_keep_alive(int fd, uint8_t id) {
    char msg[2];
    msg[0] = KEEP_ALIVE;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}

int send_ready(int fd, uint8_t id) {
    char msg[2];
    msg[0] = READY;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}

int send_input(int fd, uint8_t id, uint16_t input) {
    char msg[4];
    msg[0] = INPUT;
    msg[1] = id;
    input &= ~INPUT_BONUS1;
    memcpy(msg + 2, &input, 2);
    return write(fd, msg, 4) == -1 ? -1 : 0;
}

int send_disconnect(int fd, uint8_t id) {
    char msg[2];
    msg[0] = DISCONNECT;
    msg[1] = id;
    return write(fd, msg, 2) == -1 ? -1 : 0;
}

int parse_lobby_status(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    player_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < player_cnt; ++i) {
        data = get_bytes(reader, 25);
        if (!data) {
            return -1;
        }
        auto id = (uint8_t) data[0];
        players[i].id = id;
        strncpy(player_names[id], data + 1, 23);
        player_names[id][23] = 0;
        players[i].ready = (uint8_t) data[24];
    }
    return 0;
}

static int parse_field_data(reader_t *reader) {
    char *data = get_bytes(reader, 2);
    if (!data) {
        return -1;
    }
    field_width = (uint8_t) data[0];
    field_height = (uint8_t) data[1];
    size_t total_bytes = field_width * field_height;
    if (field) {
        free(field);
    }
    field = malloc(total_bytes);
    size_t read_bytes = 0;
    while (read_bytes < total_bytes) {
        size_t read_size = MIN(total_bytes - read_bytes, 4096);
        data = get_bytes(reader, read_size);
        if (!data) {
            return -1;
        }
        memcpy(field + read_bytes, data, read_size);
        read_bytes += read_size;
    }
    return 0;
}

int parse_game_start(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    player_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < player_cnt; ++i) {
        data = get_bytes(reader, 29);
        if (!data) {
            return -1;
        }
        auto id = (uint8_t) data[0];
        players[i].id = id;
        strncpy(player_names[id], data + 1, 23);
        player_names[id][23] = 0;
        memcpy(&players[i].x, data + 24, 2);
        memcpy(&players[i].y, data + 26, 2);
        players[i].direction = (uint8_t) data[28];
    }
    if (parse_field_data(reader) == -1) {
        return -1;
    }
    data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    dyn_timer = (uint8_t) *data;
    return 0;
}

int parse_map_update(reader_t *reader) {
    char *data = get_bytes(reader, 2);
    if (!data) {
        return -1;
    }
    uint16_t upd_count;
    memcpy(&upd_count, data, 2);
    int i;
    for (i = 0; i < upd_count; ++i) {
        data = get_bytes(reader, 3);
        if (!data) {
            return -1;
        }
        field_set(data[0], data[1], (uint8_t) data[2]);
    }
    return 0;
}

static int parse_dyn_data(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    dyn_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < dyn_cnt; ++i) {
        data = get_bytes(reader, 4);
        if (!data) {
            return -1;
        }
        memcpy(&dynamites[i].x, data, 2);
        memcpy(&dynamites[i].y, data + 2, 2);
    }
    return 0;
}

static int parse_flame_data(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    flame_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < flame_cnt; ++i) {
        data = get_bytes(reader, 2);
        if (!data) {
            return -1;
        }
        flames[i].x = (uint8_t) data[0];
        flames[i].y = (uint8_t) data[1];
    }
    return 0;
}

static int parse_pwrup_data(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    pwrup_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < pwrup_cnt; ++i) {
        data = get_bytes(reader, 3);
        if (!data) {
            return -1;
        }
        pwrups[i].x = (uint8_t) data[0];
        pwrups[i].y = (uint8_t) data[1];
        pwrups[i].type = (uint8_t) data[2];
    }
    return 0;
}

static int parse_player_data(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    player_cnt = (uint8_t) *data;
    int i;
    for (i = 0; i < player_cnt; ++i) {
        data = get_bytes(reader, 2);
        if (!data) {
            return -1;
        }
        players[i].id = (uint8_t) data[0];
        players[i].dead = (uint8_t) data[1];
        if (!players[i].dead) {
            data = get_bytes(reader, 9);
            if (!data) {
                return -1;
            }
            memcpy(&players[i].x, data, 2);
            memcpy(&players[i].y, data + 2, 2);
            players[i].direction = (uint8_t) data[4];
            players[i].active_pwrups = (uint8_t) data[5];
            players[i].power = (uint8_t) data[6];
            players[i].speed = (uint8_t) data[7];
            players[i].count = (uint8_t) data[8];
        }
    }
    return 0;
}

int parse_objects(reader_t *reader) {
    char *data = get_bytes(reader, 2);
    if (!data) {
        return -1;
    }
    memcpy(&timer, data, 2);
    if (parse_dyn_data(reader) == -1) {
        return -1;
    }
    if (parse_flame_data(reader) == -1) {
        return -1;
    }
    if (parse_pwrup_data(reader) == -1) {
        return -1;
    }
    if (parse_player_data(reader) == -1) {
        return -1;
    }
    return 0;
}

int parse_game_over(reader_t *reader) {
    char *data = get_bytes(reader, 1);
    if (!data) {
        return -1;
    }
    winner_cnt = (uint8_t) *data;
    data = get_bytes(reader, winner_cnt);
    if (!data) {
        return -1;
    }
    memcpy(winner_ids, data, winner_cnt);
    return 0;
}
