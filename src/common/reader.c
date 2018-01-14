#include "reader.h"

void reader_init(reader_t *reader, int fd) {
    reader->fd = fd;
    reader->pos = 0;
    reader->size = 0;
}

char *get_bytes(reader_t *reader, size_t n) {
    if (n > 4096) {
        return NULL;
    }
    size_t available = reader->size - reader->pos;
    if (available < n) {
        memmove(reader->buf, reader->buf + reader->pos, available);
        reader->pos = 0;
        ssize_t read_result = read(reader->fd, reader->buf + available, 4096 - available);
        if (available + read_result < n) {
            if (read_result == -1) {
                fprintf(stderr, "Failed to receive message: %s\n", strerror(errno));
            }
            reader->size = 0;
            return NULL;
        }
        reader->size = available + read_result;
    }
    size_t pos = reader->pos;
    reader->pos += n;
    return reader->buf + pos;
}
