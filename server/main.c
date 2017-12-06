#include <unistd.h>
#include "../protocol.h"
#include "setup.h"
#include "player.h"
#include "broadcast.h"

#define STATE_LOBBY 1
#define STATE_IN_PROGRESS 2
#define STATE_GAME_OVER 3

static int state = STATE_LOBBY;

void send_lobby_ready() {
    void *msg;
    size_t size = prepare_lobby_status(&msg);
    broadcast(msg, size);
    free(msg);
}

void disconnect_client(int fd, player *player) {
    unregister_client(fd);
    close(fd);
    if (player) {
        printf("%s disconnected\n", player->name);
        remove_player(player->id);
    }
    pthread_exit(NULL);
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
                    break;
                }
                response[0] = JOIN_RESPONSE;
                if (state == STATE_IN_PROGRESS || state == STATE_GAME_OVER) {
                    response[1] = JOIN_RESPONSE_BUSY;
                    send(fd, response, 2, MSG_NOSIGNAL);
                    disconnect_client(fd, player);
                }
                memmove(buf, buf + 1, 23);
                buf[23] = 0;
                player = add_player(buf);
                if (!player) {
                    response[1] = JOIN_RESPONSE_FULL;
                    send(fd, response, 2, MSG_NOSIGNAL);
                    disconnect_client(fd, player);
                }
                response[1] = JOIN_RESPONSE_SUCCESS;
                response[2] = player->id;
                if (send(fd, response, 3, MSG_NOSIGNAL) == -1) {
                    fprintf(stderr, "Failed to send message to %s: %s\n", buf, strerror(errno));
                    disconnect_client(fd, player);
                }
                register_client(fd);
                send_lobby_ready();
                printf("%s connected\n", buf);
                break;
            case READY:
                if (state != STATE_LOBBY) {
                    continue;
                }
                player->ready = (uint8_t) (player->ready ? 0 : 1);
                send_lobby_ready();
                printf("%s ready!\n", player->name);
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

    if (read_args(argc, argv, &addr) == -1) {
        return -1;
    }

    if ((fd = setup_socket(&addr, 10)) == -1) {
        return -1;
    }

    int cl_fd;
    struct sockaddr cl_addr;
    socklen_t cl_addrlen;
    pthread_t cl_thread;

    for (;;) {
        if ((cl_fd = accept(fd, &cl_addr, &cl_addrlen)) == -1) {
            fprintf(stderr, "Failed to accept incoming connection: %s\n", strerror(errno));
            continue;
        }

        if ((errno = pthread_create(&cl_thread, NULL, client_thread, &cl_fd))) {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
            close(cl_fd);
        }
    }

    return 0;
}