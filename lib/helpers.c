#include "helpers.h"
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define RERROR(_X) \
    if (_X == -1) { \
        kill(0, SIGINT); \
        free(pipefd); \
        free(childPid); \
        sigaction(SIGINT, &old_action, NULL); \
        return (sigint_catched == 1 ? 0 : -1); \
    }

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
    // dprintf(STDERR_FILENO, "Exec command %s\n", program->command);
    // int i = -1;
    // do
    // {
    //     ++i;
    //     dprintf(STDERR_FILENO, "Exec arg %s\n", program->args[i]);
    // } while (program->args[i] != NULL);

    switch (childPid = fork()) {
        case -1:
            // dprintf(STDERR_FILENO, "Forked errno %d\n", errno);
            break;
        case 0:
            execvp(program->command, program->args);
            break;
    }
    // dprintf(STDERR_FILENO, "Forked errno %d\n", errno);
    return childPid;
}

int sigint_catched = 0;

void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        sigint_catched = 1;
        while (wait(NULL) != -1)
        {
        }
    }
}

int runpiped(struct execargs_t** programs, size_t n)
{
    struct sigaction new_action;
    struct sigaction old_action;
    new_action.sa_handler = sig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigint_catched = 0;
    sigaction(SIGINT, &new_action, &old_action);
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
            // dprintf(STDERR_FILENO, "Check read dup\n");
            RERROR(dup2(pipefd[readPipe][0], STDIN_FILENO));
            // dprintf(STDERR_FILENO, "Check close read\n");
            RERROR(close(pipefd[readPipe][0]));
        }

        if (i != n - 1)
        {
            // dprintf(STDERR_FILENO, "Check create pipe\n");
            RERROR(pipe(pipefd[writePipe]));
            // dprintf(STDERR_FILENO, "Check write dup\n");
            RERROR(dup2(pipefd[writePipe][1], STDOUT_FILENO));
            // dprintf(STDERR_FILENO, "Check close write\n");
            RERROR(close(pipefd[writePipe][1]));
        }
        else
        {
            // dprintf(STDERR_FILENO, "Check dup stdout\n");
            RERROR(dup2(default_stdout, STDOUT_FILENO));
        }
        childPid[i] = exec(programs[i]);
        // dprintf(STDERR_FILENO, "Check exec\n");
        RERROR(childPid[i]);
        ++readPipe;
        ++writePipe;
    }
    // dprintf(STDERR_FILENO, "Check dup stdin\n");
    RERROR(dup2(default_stdin, STDIN_FILENO));
    size_t countPrograms = 0;
    while (countPrograms < n)
    {
        int cpid = wait(NULL);
        // dprintf(STDERR_FILENO, "Check wait %d\n", cpid);
        RERROR(cpid);
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
    sigaction(SIGINT, &old_action, NULL); \
    free(pipefd);
    free(childPid);
    return 0;
}
