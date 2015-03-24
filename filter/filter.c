#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096

int main(int argc, char* argv[]) {
    char buf[4096];

    char** argvs = (char**)malloc(sizeof(char*) * (argc + 2));
    int i;
    for (i = 0; i < argc - 1; i++)
    {
        argvs[i] = argv[i + 1];
    }
    argvs[argc] = NULL;
    while (1)
    {
        ssize_t readed = read_until(STDIN_FILENO, buf, BUF_SIZE, '\n');
        if (readed == 0)
        {
            break;
        } 
        else if (readed == -1) 
        {
            perror("error while reading");
            break;
        }
        if (buf[readed - 1] == '\n') {
            buf[readed -1] = 0;
        }
        argvs[argc - 1] = buf;
        int res = spawn(argvs[0], argvs);
        if (res == 0)
        {
            if (buf[readed - 1] == 0) {
                buf[readed - 1] = '\n';
            }
            write_(STDOUT_FILENO, buf, readed);
        }
    }
    free(argvs);
    return 0;
}
