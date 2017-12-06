#ifndef BOMBERMAN_BROADCAST_H
#define BOMBERMAN_BROADCAST_H

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>

void register_client(int fd);

void unregister_client(int fd);

void broadcast(void *msg, size_t size);

#endif //BOMBERMAN_BROADCAST_H
