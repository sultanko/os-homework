#include "helpers.h"
#include <stdio.h>

#define WORD_SIZE 1024 
#define BUF_SIZE 2048

void shift_buf(char *buf, size_t count)
{
        size_t pos = 0;
        while (buf[pos + count] != '\0')
        {
            buf[pos] = buf[pos + count];
            pos++;
        }
        buf[pos] = '\0';
}


ssize_t delwords(char* word, char* buf, size_t count)
{
    size_t pos = 0;
    while (buf[pos] != '\0')
    {
        size_t posWord = 0;
        size_t posCmp = pos;
        while (word[posWord] != '\0' && buf[posCmp] == word[posWord])
        {
            posCmp++;
            posWord++;
        }
        if (word[posWord] == '\0')
        {
            shift_buf(buf + pos, posWord);
            count -= posWord;
        }
        pos++;
    }

    return count;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 1;
    }
    char* word = argv[1];
    // printf("WORD %s\n", word);
    char buf[BUF_SIZE];
    ssize_t size_after_cmp = 0;
    while (1)
    {
        ssize_t readed = read_(STDIN_FILENO, buf + size_after_cmp, BUF_SIZE - size_after_cmp);
        if (readed == -1) 
        {
            perror("error while reading");
            return 1;
        }
        size_after_cmp = delwords(word, buf, size_after_cmp + readed);
        if (size_after_cmp == -1)
        {
            perror("error while deleting");
            return 2;
        }
        if (readed == 0)
        {
            write_(STDOUT_FILENO, buf, size_after_cmp);
            return 0;
        }
        if (size_after_cmp > WORD_SIZE)
        {
            write_(STDOUT_FILENO, buf, size_after_cmp - WORD_SIZE);
            shift_buf(buf, size_after_cmp - WORD_SIZE);
            size_after_cmp = WORD_SIZE;
        }
    }
}
