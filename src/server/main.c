#include <signal.h>
#include <time.h>
#include "utils.h"
#include "reader.h"
#include "setup.h"
#include "game.h"

#define STATE_LOBBY 1
#define STATE_PREPARING 2
#define STATE_IN_PROGRESS 3

static int state = STATE_LOBBY;

static void *loop_thread(void *arg) {
    signal(SIGPIPE, SIG_IGN);

    time_t epoch = time(NULL);
    time_t cur_time = epoch;
    int game_over = 0;

    while (!game_over) {
        time_t timestamp = time(NULL);
        uint16_t timer = (uint16_t) MAX(TIMER - (cur_time - epoch) / 1000, 0);

        pthread_mutex_lock(&players_lock);
        game_over = do_tick(timer, cur_time);
        pthread_mutex_unlock(&players_lock);

        cur_time += 1000 / TICK_RATE;
        usleep((useconds_t) MAX((timestamp + 1000 / TICK_RATE - time(NULL)) * 1000, 0));
    }

    puts("GAME OVER");
    pthread_mutex_lock(&players_lock);
    send_game_over();
    reset_game();
    pthread_mutex_unlock(&players_lock);
    puts("LOBBY STAGE");
    state = STATE_LOBBY;
    pthread_exit(NULL);
}

static void start_game(void) {
    puts("STARTING GAME");
    state = STATE_IN_PROGRESS;
    pthread_t thread;
    if ((errno = pthread_create(&thread, NULL, loop_thread, NULL))) {
        fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
    }
}

static void start_preparation(void) {
    set_players_not_ready();
    puts("PREPARATION STAGE");
    state = STATE_PREPARING;
    setup_game();
}

/*
 * side effects:
 * socket closed
 * send_lobby_status()
 * thread exit
 */
static void disconnect_client(int fd, player_t *player) {
    if (!player) {
        close(fd);
    } else {
        pthread_mutex_lock(&players_lock);
        printf("%s disconnected\n", player->name);
        remove_player(player);
        if (state == STATE_LOBBY) {
            send_lobby_status();
            if (player_count > 1 && all_players_ready()) {
                start_preparation();
            }
        } else if (state == STATE_PREPARING) {
            if (player_count < 2) {
                puts("LOBBY STAGE");
                state = STATE_LOBBY;
            } else if (all_players_ready()) {
                start_game();
            }
        } else {
            remove_player_objects(player);
        }
        pthread_mutex_unlock(&players_lock);
    }
    pthread_exit(NULL);
}

static void *client_thread(void *arg) {
    int fd = *(int *) arg;
    reader_t reader;
    reader_init(&reader, fd);
    char *data;
    uint8_t response[3];
    player_t *player = NULL;

    for (;;) {
        data = get_bytes(&reader, 1);
        if (!data) {
            disconnect_client(fd, player);
        }
        if (!player && *data != JOIN_REQUEST) {
            disconnect_client(fd, player);
        }

        switch (*data) {
            case JOIN_REQUEST:
                data = get_bytes(&reader, 23);
                if (player || !data) {
                    continue;
                }
                response[0] = JOIN_RESPONSE;
                if (state != STATE_LOBBY) {
                    response[1] = JOIN_RESPONSE_BUSY;
                    if (!send_msg(fd, response, 2)) {
                        fprintf(stderr, "Failed to send join response: %s\n", strerror(errno));
                    }
                    disconnect_client(fd, player);
                }
                if (player_count + 1 > MAX_PLAYERS) {
                    response[1] = JOIN_RESPONSE_FULL;
                    if (!send_msg(fd, response, 2)) {
                        fprintf(stderr, "Failed to send join response: %s\n", strerror(errno));
                    }
                    disconnect_client(fd, player);
                }

                pthread_mutex_lock(&players_lock);
                player = add_player(fd, data);
                pthread_mutex_unlock(&players_lock);

                if (!player) {
                    response[1] = JOIN_RESPONSE_FULL;
                    if (!send_msg(fd, response, 2)) {
                        fprintf(stderr, "Failed to send join response: %s\n", strerror(errno));
                    }
                    disconnect_client(fd, player);
                }
                response[1] = JOIN_RESPONSE_SUCCESS;
                response[2] = player->id;
                if (!send_msg(fd, response, 3)) {
                    fprintf(stderr, "Failed to send message to %s: %s\n", player->name, strerror(errno));
                    disconnect_client(fd, player);
                }

                printf("%s connected\n", player->name);
                pthread_mutex_lock(&players_lock);
                send_lobby_status();
                pthread_mutex_unlock(&players_lock);
                break;
            case READY:
                data = get_bytes(&reader, 1);
                if (!data || *data != player->id) {
                    continue;
                }
                if (state == STATE_LOBBY) {
                    player->ready = (uint8_t) (player->ready ? 0 : 1);
                    printf("%s%s ready!\n", player->name, player->ready ? "" : " not");

                    pthread_mutex_lock(&players_lock);
                    send_lobby_status();
                    if (player_count > 1 && all_players_ready() && state == STATE_LOBBY) {
                        start_preparation();
                    }
                    pthread_mutex_unlock(&players_lock);
                } else if (state == STATE_PREPARING && !player->ready) {
                    player->ready = 1;
                    printf("%s ready!\n", player->name);

                    pthread_mutex_lock(&players_lock);
                    if (player_count > 1 && all_players_ready() && state == STATE_PREPARING) {
                        start_game();
                    }
                    pthread_mutex_unlock(&players_lock);
                }
                break;
            case INPUT:
                data = get_bytes(&reader, 3);
                if (state != STATE_IN_PROGRESS || !data || *data != player->id || player->dead) {
                    continue;
                }
                uint16_t input;
                memcpy(&input, data + 1, 2);
                if (input & INPUT_PLANT && !(player->input & INPUT_PLANT)) {
                    player->plant_pressed = 1;
                }
                if (input & INPUT_DETONATE && !(player->input & INPUT_DETONATE) && player->active_pwrups & ACTIVE_PWRUP_REMOTE) {
                    player->detonate_pressed = 1;
                }
                if (input & INPUT_PICK_UP && !(player->input & INPUT_PICK_UP)) {
                    player->pick_up_pressed = 1;
                }
                player->input = input;
                break;
            case DISCONNECT:
                data = get_bytes(&reader, 1);
                if (!data || *data != player->id) {
                    continue;
                }
                disconnect_client(fd, player);
            case KEEP_ALIVE:
                get_bytes(&reader, 1);
                break;
            default:
                fprintf(stderr, "Received unknown packet type %d from %s\n", *data, player->name);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    int fd;
    struct sockaddr_in addr;

    signal(SIGPIPE, SIG_IGN);
    srand((unsigned) time(NULL));

    if (read_args(argc, argv, &addr) == -1) {
        return -1;
    }

    if ((fd = setup_socket(&addr, 10)) == -1) {
        return -1;
    }

    puts("LOBBY STAGE");

    int cl_fd;
    pthread_t cl_thread;
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    for (;;) {
        if ((cl_fd = accept(fd, NULL, NULL)) == -1) {
            fprintf(stderr, "Failed to accept incoming connection: %s\n", strerror(errno));
            continue;
        }

        if (setsockopt(cl_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            fprintf(stderr, "Failed to set socket timeout: %s\n", strerror(errno));
            continue;
        }

        if ((errno = pthread_create(&cl_thread, NULL, client_thread, &cl_fd))) {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
            close(cl_fd);
        }
    }
}
