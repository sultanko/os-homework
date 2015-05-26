#include <stdlib.h>
#include "helpers.h"

#include <stdlib.h>
#include <stdio.h>

#define BUF_SIZE 4096

#define FERROR(_X) if ((_X) == -1) { _exit(EXIT_FAILURE); }

int main(int argc, char* argv[]) {
    char buf[BUF_SIZE];

    while (1)
    {
        write(STDOUT_FILENO, "$", 1);
        int readed=read_until(STDIN_FILENO, buf, BUF_SIZE, '\n');
        if (readed == 0)
        {
            break;
        }
        FERROR(readed);
        int countPrograms = 1;
        for (int i = 1; i < readed; ++i)
        {
            if (buf[i] == '|')
            {
                ++countPrograms;
            }
        }
        struct execargs_t** programs = (struct execargs_t**)malloc(sizeof(struct execargs_t*) * countPrograms);
        int last_command_index = 0;
        int last_args_index = 0;
        int countArgs = 0;
        char* program = NULL;
        int index_program = 0;
        for (int i = 1; i < readed; ++i)
        {
            if (buf[i] == ' ' && buf[i-1] != '\0')
            {
                buf[i] = '\0';
                ++countArgs;
            }
            else if (buf[i] == '|' || buf[i] == '\n')
            {
                buf[i] = '\0';
                char** command_args = (char**) malloc(sizeof(char*)*(countArgs+1));
                command_args[countArgs] = NULL;
                last_args_index = last_command_index;
                countArgs = 0;
                for (int j = last_command_index; j <= i; ++j)
                {
                    if (buf[j] == '\0' && j > 0 && buf[j-1] != '\0')
                    {
                        command_args[countArgs++] = buf + last_args_index;
                    }
                    if (buf[j] == '\0')
                    {
                        last_args_index = j + 1;
                    }
                }
                struct execargs_t* cur_program = (struct execargs_t*)malloc(sizeof(struct execargs_t));
                cur_program->command = command_args[0];
                cur_program->args = command_args;
                programs[index_program++] = cur_program;
                countArgs = 0;
                last_command_index = i;
                while (last_command_index < readed && 
                        (buf[last_command_index] == ' ' 
                         || buf[last_command_index] == '|'
                         || buf[last_command_index] == '\0'))
                {
                    ++last_command_index;
                }
            }
        }
        for (int ip = 0; ip < index_program; ++ip)
        {
            printf("%s\n", programs[ip]->command);
            for (int ia = 0; programs[ip]->args[ia] != NULL; ++ia)
            {
                printf("%s\n", programs[ip]->args[ia]);
            }
        }
        int result = runpiped(programs, index_program);
    }
}


