#include "bufio.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#define BUF_SIZE 4096

int main()
{
    struct buf_t* buf = buf_new(BUF_SIZE);
    while (1)
    {
        ssize_t readed = buf_fill(STDIN_FILENO, buf, BUF_SIZE);
        if (readed == -1) 
        {
            perror("error while reading");
            break;
        }
        if (buf_flush(STDOUT_FILENO, buf, readed) == -1)
        {
            perror("error while writing");
            break;
        }
        if (readed < BUF_SIZE)
        {
            break;
        } 
    }
    buf_free(buf);
}
