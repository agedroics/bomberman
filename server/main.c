#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "../protocol.h"
#include "setup.h"
#include "game.h"

#define STATE_LOBBY 1
#define STATE_PREPARING 2
#define STATE_IN_PROGRESS 3
#define STATE_GAME_OVER 4

static int state = STATE_LOBBY;

/*
 * side effects:
 * socket closed
 * send_lobby_status()
 * thread exit
 */
void disconnect_client(int fd, player *player) {
    if (!player) {
        close(fd);
    } else {
        lock_players();
        printf("%s disconnected\n", player->name);
        remove_player(player);
        if (state == STATE_LOBBY) {
            send_lobby_status();
        }
        unlock_players();
    }
    pthread_exit(NULL);
}

void *loop_thread(void *arg) {

}

void *client_thread(void *arg) {
    int fd = *(int *) arg;
    ssize_t bytes_read;
    char buf[24];
    uint8_t response[3];
    player *player = NULL;

    for (;;) {
        bytes_read = read(fd, buf, 24);
        if (bytes_read == -1) {
            fprintf(stderr, "Failed to receive message: %s\n", strerror(errno));
            disconnect_client(fd, player);
        } else if (!bytes_read) {
            disconnect_client(fd, player);
        }

        if (!player && *buf != JOIN_REQUEST) {
            disconnect_client(fd, player);
        }

        switch (*buf) {
            case JOIN_REQUEST:
                if (player) {
                    continue;
                }
                response[0] = JOIN_RESPONSE;
                if (state != STATE_LOBBY) {
                    response[1] = JOIN_RESPONSE_BUSY;
                    write(fd, response, 2);
                    disconnect_client(fd, player);
                }
                if (player_count + 1 > max_players) {
                    response[1] = JOIN_RESPONSE_FULL;
                    write(fd, response, 2);
                    disconnect_client(fd, player);
                }
                memmove(buf, buf + 1, 23);
                buf[23] = 0;

                lock_players();
                player = add_player(fd, buf);
                unlock_players();

                if (!player) {
                    response[1] = JOIN_RESPONSE_FULL;
                    write(fd, response, 2);
                    disconnect_client(fd, player);
                }
                response[1] = JOIN_RESPONSE_SUCCESS;
                response[2] = player->id;
                if (write(fd, response, 3) == -1) {
                    fprintf(stderr, "Failed to send message to %s: %s\n", player->name, strerror(errno));
                    disconnect_client(fd, player);
                }

                printf("%s connected\n", player->name);
                lock_players();
                send_lobby_status();
                unlock_players();

                break;
            case READY:
                // TODO: handle STATE_PREPARING
                if (state == STATE_LOBBY) {
                    player->ready = (uint8_t) (player->ready ? 0 : 1);
                    printf("%s%s ready!\n", player->name, player->ready ? " ready" : "");
                    lock_players();
                    send_lobby_status();
                    if (all_players_ready()) {
                        state = STATE_PREPARING;
                        puts("PREPARATION STAGE");
                        set_players_not_ready();
                        send_game_start();
                    }
                    unlock_players();
                } else if (state == STATE_PREPARING) {
                    player->ready = (uint8_t) (player->ready ? 0 : 1);
                    printf("%s%s ready!\n", player->name, player->ready ? " ready" : "");
                    lock_players();
                    if (all_players_ready()) {
                        state = STATE_IN_PROGRESS;
                        puts("STARTING GAME");
                        set_players_not_ready();
                        pthread_t thread;
                        if ((errno = pthread_create(&thread, NULL, loop_thread, NULL))) {
                            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
                        }
                    }
                    unlock_players();
                }
                break;
            case INPUT:
                if (state != STATE_IN_PROGRESS) {
                    continue;
                }

                memcpy(&player->input, buf + 2, 2);

                break;
            case DISCONNECT:
                disconnect_client(fd, player);
            default:
                continue;
        }
    }
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

    return 0;
}