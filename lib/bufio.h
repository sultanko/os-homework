#ifndef BUFIO_H_
#define BUFIO_H_

#include <unistd.h>
#include <stdlib.h>

struct buf_t
{
    size_t size;
    size_t capacity;
    char* data;
};

struct buf_t* buf_new(size_t capacity);
void buf_free(struct buf_t* buf);
size_t buf_capacity(struct buf_t* buf);
size_t buf_size(struct buf_t* buf);

ssize_t buf_fill(int fd, struct buf_t *buf, size_t required);
ssize_t buf_flush(int fd, struct buf_t *buf, size_t required);
#endif
