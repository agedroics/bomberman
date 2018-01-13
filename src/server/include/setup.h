#ifndef BOMBERMAN_SETUP_H
#define BOMBERMAN_SETUP_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int read_args(int argc, char **argv, struct sockaddr_in *addr);

int setup_socket(struct sockaddr_in *addr, int backlog);

#endif //BOMBERMAN_SETUP_H
