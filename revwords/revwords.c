#include "helpers.h"
#include <stdio.h>

#define BUF_SIZE 4097

ssize_t revwords(char* buf, size_t count)
{
    size_t i = 0;
    size_t endDel = 0;
    if (count > 0 && buf[count - 1] == ' ')
    {
        endDel = 1;
    }
    while (2 * i < count - endDel)
    {
        char c = buf[i];
        buf[i] = buf[count - i - 1 - endDel];
        buf[count - i - 1 - endDel] = c;
        i++;
    }
        
    write_(STDOUT_FILENO, buf, count);
}

int main()
{
    char buf[BUF_SIZE];
    while (1)
    {
        ssize_t readed = read_until(STDIN_FILENO, buf, BUF_SIZE, ' ');
        if (readed == 0)
        {
            return 0;
        } 
        else if (readed == -1) 
        {
            perror("error while reading");
            return 1;
        }
        else if (revwords(buf, readed) ==  -1) 
        {
            perror("error while writing");
            return 2;
        }
    }
}
