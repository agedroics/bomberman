#include "broadcast.h"

static int *cl_fds = NULL;
static char *taken_slots = NULL;
static int cl_count = 0;
static size_t arr_len = 0;
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

void register_client(int fd) {
    pthread_mutex_lock(&clients_lock);
    if (!cl_fds) {
        cl_fds = malloc(8 * sizeof(int));
        taken_slots = calloc(8, sizeof(char));
        arr_len = 8;
    } else if (cl_count + 1 > arr_len) {
        cl_fds = realloc(cl_fds, arr_len * 2);
        taken_slots = realloc(taken_slots, arr_len * 2);
        arr_len *= 2;
    }
    int i;
    for (i = 0; i < arr_len && taken_slots[i]; ++i);
    cl_fds[i] = fd;
    taken_slots[i] = 1;
    ++cl_count;
    pthread_mutex_unlock(&clients_lock);
}

void _unregister_client (int i) {
    pthread_mutex_lock(&clients_lock);
    taken_slots[i] = 0;
    --cl_count;
    pthread_mutex_unlock(&clients_lock);
}

void unregister_client(int fd) {
    int i;
    for (i = 0; i < arr_len; ++i) {
        if (cl_fds[i] == fd && taken_slots[i]) {
            _unregister_client(i);
            return;
        }
    }
}

void broadcast(void *msg, size_t size) {
    int i;
    for (i = 0; i < arr_len; ++i) {
        if (taken_slots[i]) {
            if (send(cl_fds[i], msg, size, MSG_NOSIGNAL) == -1) {
                _unregister_client(i);
            }
        }
    }
}