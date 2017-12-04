#include <unistd.h>
#include "../common/protocol.h"
#include "setup.h"
#include "player.h"

#define STATE_LOBBY 1
#define STATE_GAME 2
#define STATE_OVER 3

int state = STATE_LOBBY;

pthread_mutex_t players_lock;
int max_players = 8;

void disconnect_client(int fd, int id) {
    close(fd);
    pthread_mutex_lock(&players_lock);
    if (id != -1) {
        taken_slots[id] = 0;
        player_count--;
        printf("%s disconnected\n", players[id].name);
    }
    pthread_mutex_unlock(&players_lock);
    pthread_exit(NULL);
}

void *client_thread(void *arg) {
    int fd = *(int *) arg;
    ssize_t bytes_read;
    unsigned char buf[24];
    unsigned char response[3];
    char name[24];
    int id = -1;

    for (;;) {
        bytes_read = read(fd, buf, 24);
        if (bytes_read == -1) {
            fprintf(stderr, "Failed to receive message: %s\n", strerror(errno));
            disconnect_client(fd, id);
        } else if (!bytes_read) {
            disconnect_client(fd, id);
        }

        if (id == -1 && *buf != JOIN_REQUEST) {
            disconnect_client(fd, id);
        }

        switch (*buf) {
            case JOIN_REQUEST:
                if (id != -1) break;
                response[0] = JOIN_RESPONSE;
                if (state == STATE_GAME || state == STATE_OVER) {
                    response[1] = JOIN_RESPONSE_BUSY;
                    write(fd, response, 2);
                    disconnect_client(fd, id);
                } else if (player_count >= max_players) {
                    response[1] = JOIN_RESPONSE_FULL;
                    write(fd, response, 2);
                    disconnect_client(fd, id);
                }
                memcpy(name, buf + 1, 23);
                name[23] = 0;
                id = add_player(name);
                response[1] = JOIN_RESPONSE_SUCCESS;
                response[2] = (unsigned char) id;
                if (write(fd, response, 3) == -1) {
                    fprintf(stderr, "Failed to send message to %s: %s\n", name, strerror(errno));
                    disconnect_client(fd, id);
                }
                printf("%s connected\n", name);
                break;
            case READY:
                players[id].ready = (uint8_t) (players[id].ready ? 0 : 1);
                printf("%s ready!\n", name);
                break;
            case INPUT:
                memcpy(&players[id].input, buf + 2, 2);
                break;
            case DISCONNECT:
                disconnect_client(fd, id);
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

    int mutex_init_retval;
    if ((mutex_init_retval = pthread_mutex_init(&players_lock, NULL))) {
        fprintf(stderr, "Failed to init mutex: %s\n", strerror(mutex_init_retval));
        return -1;
    }

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