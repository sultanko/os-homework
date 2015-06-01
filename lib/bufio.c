#include "bufio.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef DEBUG
    #define CHECK assert(buf!=NULL);
#else
    #define CHECK 
#endif

struct buf_t* buf_new(size_t capacity)
{
    struct buf_t* buf = (struct buf_t*)malloc(sizeof(struct buf_t));
    if (buf != NULL)
    {
        buf->data = malloc(capacity);
        buf->capacity = capacity;
        buf->size = 0;
        if (buf->data == NULL) {
            free(buf);
            buf = NULL;
        }
    }
    return buf;
}

void buf_free(struct buf_t *buf)
{
    CHECK

    free(buf->data);
    free(buf);
}

size_t buf_capacity(struct buf_t *buf)
{
    CHECK
    return buf->capacity;
}

size_t buf_size(struct buf_t *buf)
{
    CHECK
    return buf->size;
}

ssize_t buf_fill(int fd, struct buf_t *buf, size_t required)
{
    CHECK
    ssize_t readed = -1;
    while (buf->size < required && buf->size < buf->capacity) 
    {
        readed = read(fd, buf->data + buf->size, buf->capacity - buf->size);
        if (readed == -1)
        {
            return -1;
        }
        if (readed == 0)
        {
            break;
        }
        buf->size += readed;
    }
    return buf->size;
}

ssize_t buf_flush(int fd, struct buf_t *buf, size_t required)
{
    CHECK
    size_t totalWritten = 0;
    if (required > buf->size)
    {
        return -1;
    }
    while (totalWritten < required && buf->size > 0)
    {
        ssize_t written = write(fd, buf->data + totalWritten, buf->size);
        if (written == -1)
        {
            memmove(buf, buf + totalWritten, buf->size);
            return -1;
        }
        totalWritten += written;
        buf->size -= written;
    }
    if (buf->size > 0)
    {
        memmove(buf, buf + totalWritten, buf->size);
    }
    return totalWritten;
}
