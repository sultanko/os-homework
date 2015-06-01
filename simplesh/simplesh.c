#include <stdlib.h>
#include <helpers.h>
#include <bufio.h>

#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define BUF_SIZE 4096

#define FERROR(_X) if ((_X) == -1) { \
    exit(EXIT_FAILURE); }

int sigint_catched_sh = 0;

void sig_handler_sh(int signo)
{
    if (signo == SIGINT)
    {
        sigint_catched_sh = 1;
        return;
    }
}

void parse_line(char *buf, int buf_len,
        struct execargs_t** programs, struct execargs_t* programs_array)
{
    int last_command_index = 0;
    int last_args_index = 0;
    int count_args = 0;
    int index_program = 0;
    for (int i = 0; i < buf_len; ++i)
    {
        if (buf[i] == ' ')
        {
            buf[i] = '\0';
        }
        if (buf[i] == '|' || buf[i] == '\n')
        {
            buf[i] = '\0';
            count_args = (last_command_index == 0 && buf[0] != '\0') ? 1 : 0;
            for (int j = last_command_index; j < i; ++j)
            {
                if (j > 0 && buf[j - 1] == '\0' && buf[j] != '\0')
                {
                    count_args++;
                }
            }
            char** command_args = (char**) malloc(sizeof(char*)*(count_args+1));
            command_args[count_args] = NULL;
            last_args_index = last_command_index;
            count_args = 0;
            for (int j = last_command_index; j <= i; ++j)
            {
                if (buf[j] == '\0' && j > 0 && buf[j-1] != '\0')
                {
                    command_args[count_args++] = buf + last_args_index;
                }
                if (buf[j] == '\0')
                {
                    last_args_index = j + 1;
                }
            }
            struct execargs_t* cur_program = programs_array + index_program;
            cur_program->command = command_args[0];
            cur_program->args = command_args;
            programs[index_program++] = cur_program;
            count_args = 0;
            last_command_index = i;
            while (last_command_index < buf_len && 
                    (buf[last_command_index] == ' ' 
                     || buf[last_command_index] == '|'
                     || buf[last_command_index] == '\0'))
            {
                ++last_command_index;
            }
        }
    }
}



int main(int argc, char* argv[]) {

    struct sigaction new_action;
    new_action.sa_handler = sig_handler_sh;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, &new_action, NULL);
    
    struct buf_t* read_buffer = buf_new(BUF_SIZE);
    char* buf = (char*)read_buffer->data;
    int dev_null_fd = open("/dev/null", O_WRONLY);

    while (1)
    {
        FERROR(write(STDOUT_FILENO, "$", 1));
        int readed = -1;
        while (readed == -1)
        {
            int cur_size = buf_fill(STDIN_FILENO, read_buffer, buf_size(read_buffer) + 1);
            if (cur_size == 0)
            {
                readed = 0;
                break;
            }
            if (cur_size == -1)
            {
                if (errno == EINTR && sigint_catched_sh == 1)
                {
                    write(STDOUT_FILENO, "\n$", 2);
                    errno = 0;
                    sigint_catched_sh = 0;
                    buf_flush(dev_null_fd, read_buffer, buf_size(read_buffer));
                    continue;
                }
            }
            for (int i = 0; i < cur_size; ++i)
            {
                if (buf[i] == '\n')
                {
                    readed = i + 1;
                    break;
                }
            }
        }
        if (readed == 0)
        {
            break;
        }
        FERROR(readed);
        int countPrograms = 1;
        for (int i = 0; i < readed; ++i)
        {
            if (buf[i] == '|')
            {
                ++countPrograms;
            }
        }
        struct execargs_t* programs_array = (struct execargs_t*)malloc(sizeof(struct execargs_t) * countPrograms);
        struct execargs_t** programs = (struct execargs_t**)malloc(sizeof(struct execargs_t*) * countPrograms);
        parse_line(buf, readed, programs, programs_array);
        int result = runpiped(programs, countPrograms);
        for (int i = 0; i < countPrograms; ++i)
        {
            free(programs[i]->args);
        }
        free(programs_array);
        free(programs);
        FERROR(result);
        FERROR(buf_flush(dev_null_fd, read_buffer, readed));
    }
    buf_free(read_buffer);
    close(dev_null_fd);
    exit(EXIT_SUCCESS);
}


