#include <stdlib.h>
#include <helpers.h>
#include <bufio.h>

#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#define BUF_SIZE 4096

#define FERROR(_X) if ((_X) == -1) { _exit(EXIT_FAILURE); }


int main(int argc, char* argv[]) {
    
    struct buf_t* read_buffer = buf_new(BUF_SIZE);
    char* buf = (char*)read_buffer->data;
    int devNull = open("/dev/null", O_WRONLY);

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
            FERROR(cur_size);
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
        int last_command_index = 0;
        int last_args_index = 0;
        int countArgs = 0;
        char* program = NULL;
        int index_program = 0;
        for (int i = 0; i < readed; ++i)
        {
            if (buf[i] == ' ')
            {
                buf[i] = '\0';
            }
            if (buf[i] == '|' || buf[i] == '\n')
            {
                buf[i] = '\0';
                countArgs = (last_command_index == 0 && buf[0] != '\0') ? 1 : 0;
                for (int j = last_command_index; j < i; ++j)
                {
                    if (j > 0 && buf[j - 1] == '\0' && buf[j] != '\0')
                    {
                        countArgs++;
                    }
                }
                // dprintf(STDERR_FILENO, "Count args %d\n", countArgs);
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
                struct execargs_t* cur_program = programs_array + index_program;
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
        int result = runpiped(programs, index_program);
        FERROR(buf_flush(devNull, read_buffer, readed));
        // dprintf(STDOUT_FILENO, "Result runpiped %d\n", result);
    }
    buf_free(read_buffer);
    close(devNull);
    exit(EXIT_SUCCESS);
}


