#define _GNU_SOURCE
#include "helpers.h"
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define RERROR(_X) \
    if (_X == -1) { \
        int old_trigger = sigint_catched_runpipe; \
        if (sigint_catched_runpipe == 0) { kill(0, SIGINT); } \
        sigaction(SIGINT, &old_action, NULL); \
        return (old_trigger == 0 ? -1 : 0); \
    }

ssize_t read_(int fd, void* buf, size_t count) 
{
    size_t readed = 0;
    ssize_t readCount = 0;
    while (readed < count)
    {
        readCount = read(fd, (char*)buf + readed, count - readed);
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
        writeCount = write(fd, (char*)buf + writted, count - writted);
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
            break;
        case 0:
            execvp(program->command, program->args);
            exit(EXIT_FAILURE);
    }
    return childPid;
}

int sigint_catched_runpipe = 0;

void sig_handler_runpiped(int signo)
{
    if (signo == SIGINT)
    {
        sigint_catched_runpipe = 1;
        while (wait(NULL) != -1)
        {
        }
    }
}

int runpiped(struct execargs_t** programs, size_t n)
{
    struct sigaction new_action;
    struct sigaction old_action;
    new_action.sa_handler = sig_handler_runpiped;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigint_catched_runpipe = 0;
    sigaction(SIGINT, &new_action, &old_action);
    int pipefd[2];
    int default_stdin = dup(STDIN_FILENO);
    int default_stdout = dup(STDOUT_FILENO);
    for (size_t i = 0; i < n; ++i)
    {
        if (i != 0)
        {
            RERROR(dup2(pipefd[0], STDIN_FILENO));
            RERROR(close(pipefd[0]));
        }

        if (i != n - 1)
        {
            RERROR(pipe2(pipefd, O_CLOEXEC))
            RERROR(dup2(pipefd[1], STDOUT_FILENO));
            RERROR(close(pipefd[1]));
        }
        else
        {
            RERROR(dup2(default_stdout, STDOUT_FILENO));
        }
        RERROR(exec(programs[i]));
    }
    RERROR(dup2(default_stdin, STDIN_FILENO));
    size_t dead_childs = 0;
    int status = 0;
    int result = 0;
    while (dead_childs < n)
    {
        int cpid = wait(&status);
        RERROR(cpid);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            result = -1;
        }
        ++dead_childs;
    }
    sigaction(SIGINT, &old_action, NULL);
    return result;
}
