#include "setup.h"

int read_args(int argc, char **argv, struct sockaddr_in *addr) {
    int i;
    int ip_set = 0;
    int port_set = 0;
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-ip")) {
            if (!inet_pton(AF_INET, argv[++i], &addr->sin_addr)) {
                fprintf(stderr, "Invalid IP address: %s\n", argv[i]);
                return -1;
            }
            ip_set = 1;
        } else if (!strcmp(argv[i], "-p")) {
            addr->sin_port = htons((uint16_t) strtol(argv[++i], NULL, 10));
            port_set = 1;
        }
    }
    if (!ip_set || !port_set) {
        fprintf(stderr, "Usage: %s -ip addr -p port\n", argv[0]);
        return -1;
    }
    return 0;
}

int setup_socket(struct sockaddr_in *addr, int backlog) {
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
        fprintf(stderr, "Failed to set socket flags: %s\n", strerror(errno));
        return -1;
    }
    if (bind(fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
        return -1;
    }
    if (listen(fd, backlog) == -1) {
        fprintf(stderr, "Failed to mark socket as passive: %s\n", strerror(errno));
        return -1;
    }
    char in_addr[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr->sin_addr, in_addr, sizeof(struct sockaddr_in))) {
        fprintf(stderr, "Failed to read ip address: %s\n", strerror(errno));
        return -1;
    }
    printf("Listening on %s:%u\n", in_addr, ntohs(addr->sin_port));
    return fd;
}