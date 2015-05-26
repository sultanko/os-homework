#include "helpers.h"
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define FERROR(_X) if ((_X) == -1) { _exit(EXIT_FAILURE); }

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
    dprintf(STDERR_FILENO, "%s arg: %s\n", file, argv[1]);
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
    // dprintf(STDERR_FILENO, "Spawn done %d\n", status);
    return status;
}

int exec(struct execargs_t* program)
{
    pid_t childPid = -1;
    switch (childPid = fork()) {
        case -1:
            return -1;
            break;
        case 0:
            execvp(program->command, program->args);
        default:
            break;
    }
    // dprintf(STDERR_FILENO, "Spawn done %d\n", status);
    return childPid;
}

int runpiped(struct execargs_t** programs, size_t n)
{
    // dprintf(STDOUT_FILENO, "Start runpiped\n");
    int** pipefd = (int**)malloc(sizeof(int*) * (n-1));
    int* childPid = (int*)malloc(sizeof(int*) * n);
    for (size_t i = 0; i < n-1; ++i)
    {
        pipefd[i] = (int*)malloc(sizeof(int)*2);
        FERROR(pipe(pipefd[i]));
    }
    // dprintf(STDOUT_FILENO, "Create pipes\n");
    int default_stdin = dup(STDIN_FILENO);
    int default_stdout = dup(STDOUT_FILENO);
    int readPipe = -1;
    int writePipe = 0;
    for (size_t i = 0; i < n; ++i)
    {
        // dprintf(STDOUT_FILENO, "First dup\n");
        if (i != 0)
        {
            // dup2(pipefd[readPipe][0], STDIN_FILENO);
            // close(STDIN_FILENO);
            dup2(pipefd[readPipe][0], STDIN_FILENO);
            close(pipefd[readPipe][0]);
        }

        if (i != n - 1)
        {
            // dup2(pipefd[writePipe][1], STDOUT_FILENO);
            // close(STDOUT_FILENO);
            dup2(pipefd[writePipe][1], STDOUT_FILENO);
            close(pipefd[writePipe][1]);
            // dprintf(STDERR_FILENO, "Second dup\n");
        }
        else
        {
            close(STDOUT_FILENO);
            dup2(default_stdout, STDOUT_FILENO);
        }
        // dprintf(STDERR_FILENO, "Exec %s\n", programs[i]->command);
        childPid[i] = exec(programs[i]);
        // dprintf(STDERR_FILENO, "Executed %s %d\n", programs[i]->command, childPid[i]);
        // if (readPipe != -1)
        // {
        //     close(pipefd[readPipe][0]);
        // }
        // if (i != n-1)
        // {
        //     close(pipefd[writePipe][1]);
        // }
        ++readPipe;
        ++writePipe;
    }
    dup2(default_stdin, STDIN_FILENO);
    size_t countPrograms = 0;
    int status = 0;
    for (size_t cp = 0; cp < n; ++cp)
    {
        int cpid = waitpid(childPid[cp], &status, 0);
        for (size_t i = 0; i < n; ++i)
        {
            if (cpid == childPid[i])
            {
                // dprintf(STDERR_FILENO, "Waited %d %zu\n", cpid, i);
                // if (i != 0) { close(pipefd[i-1][0]); }
                // if (i != n - 1) { close(pipefd[i][1]); }
                break;
            }
        }
        ++countPrograms;
    }
}
