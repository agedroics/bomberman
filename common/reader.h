#ifndef BOMBERMAN_READER_H
#define BOMBERMAN_READER_H

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

typedef struct {
    int fd;
    size_t pos;
    size_t size;
    char buf[4096];
} reader_t;

void reader_init(reader_t *reader, int fd);

char *get_bytes(reader_t *reader, size_t n);

#endif //BOMBERMAN_READER_H
