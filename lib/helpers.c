#include "helpers.h"

ssize_t read_(int fd, void* buf, size_t count) 
{
    size_t readed = 0;
    ssize_t readCount = 0;
    while (readed < count)
    {
        readCount = read(fd, buf + readed, count);
        if (readCount == -1 || readCount == 0)
        {
            break;
        }
        readed += readCount;
    }
    if (readCount == -1)
    {
        return readCount;
    }
    char* cbuf = (char*)buf;
    cbuf[readed] = '\0';
    return readed;
}

ssize_t write_(int fd, void* buf, size_t count)
{
    size_t writted = 0;
    ssize_t writeCount = 0;
    while (writted < count)
    {
        writeCount = write(fd, buf + writted, count - writted);
        if (writeCount == -1)
        {
            return -1;
        }
        writted += writeCount;
    }
    return writted;
}
