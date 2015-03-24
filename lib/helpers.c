#include "helpers.h"
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>

ssize_t read_(int fd, void* buf, size_t count) 
{
    size_t readed = 0;
    ssize_t readCount = 0;
    while (readed < count)
    {
        readCount = read(fd, buf + readed, count - readed);
        if (readCount == -1) 
        {
            return -1;
        }
        else if (readCount == 0)
        {
            break;
        }
        readed += readCount;
    }
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

ssize_t read_until(int fd, void* buf, size_t count, char delimeter)
{
    ssize_t readCount = 0;
    size_t readed = 0;
    char chr;
    char* array = (char*)buf;
    while (readed < count)
    {
        readCount = read_(fd, &chr, 1);
        if (readCount < 0)
        {
            return readCount;
        }
        if (readCount == 0)
        {
            break;
        }
        array[readed++] = chr;
        if (chr == delimeter)
        {
            break;
        }
    }
    return readed;
}

int spawn(const char* file, char* const argv[]) 
{
    int status = 0;
    pid_t childPid = -1;
    switch (childPid = fork()) {
        case -1:
            status = -1;
            break;
        case 0:
            execvp(file, argv);
            _exit(127);
        default:
            while (waitpid(childPid, &status, 0) == -1) {
                if (errno != EINTR) {
                    status = -1;
                    break;
                }
            }
            break;
    }
    return status;
}
