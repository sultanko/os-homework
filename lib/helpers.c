#include "helpers.h"
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define FERROR(_X) if ((_X) == -1) { return -1; }

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
    int default_stderr = dup(STDERR_FILENO);
    int devNull = open("/dev/null", O_WRONLY);
    switch (childPid = fork()) {
        case -1:
            status = -1;
            break;
        case 0:
            dup2(devNull, STDERR_FILENO);
            execvp(file, argv);
        default:
            while (waitpid(childPid, &status, 0) == -1) {
                if (errno != EINTR) {
                    status = -1;
                    break;
                }
            }
            break;
    }
    if (devNull != -1) {
        close(devNull);
    }
    dup2(default_stderr, STDERR_FILENO);
    return status;
}

int exec(struct execargs_t* program)
{
    pid_t childPid = -1;
    switch (childPid = fork()) {
        case -1:
            return -1;
        case 0:
            execvp(program->command, program->args);
            break;
        default:
            return childPid;
    }
}

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        while (wait(NULL) != -1)
        {
        }
    }
}

int runpiped(struct execargs_t** programs, size_t n)
{
    signal(SIGINT, sig_handler);
    int (*pipefd)[2] = (int(*)[2])malloc(sizeof(int[2]) * (n-1));
    int* childPid = (int*)malloc(sizeof(int*) * n);
    int default_stdin = dup(STDIN_FILENO);
    int default_stdout = dup(STDOUT_FILENO);
    int readPipe = -1;
    int writePipe = 0;
    for (size_t i = 0; i < n; ++i)
    {
        if (i != 0)
        {
            FERROR(dup2(pipefd[readPipe][0], STDIN_FILENO));
            FERROR(close(pipefd[readPipe][0]));
        }

        if (i != n - 1)
        {
            FERROR(pipe(pipefd[writePipe]));
            FERROR(dup2(pipefd[writePipe][1], STDOUT_FILENO));
            FERROR(close(pipefd[writePipe][1]));
        }
        else
        {
            FERROR(dup2(default_stdout, STDOUT_FILENO));
        }
        childPid[i] = exec(programs[i]);
        ++readPipe;
        ++writePipe;
    }
    FERROR(dup2(default_stdin, STDIN_FILENO));
    size_t countPrograms = 0;
    int status = 0;
    while (countPrograms < n)
    {
        int cpid = wait(&status);
        FERROR(cpid);
        if (countPrograms == 0)
        {
            for (size_t i = 0; i < n; ++i)
            {
                if (cpid != childPid[i])
                {
                    kill(childPid[i], SIGINT); 
                }
            }
        }
        ++countPrograms;
    }
    free(pipefd);
    free(childPid);
}
