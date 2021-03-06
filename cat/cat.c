#include "helpers.h"
#include <stdio.h>

#define BUF_SIZE 4096

int main()
{
    char buf[BUF_SIZE];
    while (1)
    {
        ssize_t readed = read_(STDIN_FILENO, buf, BUF_SIZE);
        if (readed == 0)
        {
            return 0;
        } 
        else if (readed == -1) 
        {
            perror("error while reading");
            return 1;
        }
        else if (write_(STDOUT_FILENO, buf, readed) == -1) 
        {
            perror("error while writing");
            return 2;
        }
    }
}
