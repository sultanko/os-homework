#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <bufio.h>
#include <stdio.h>
#include <sys/wait.h>

#define BUF_SIZE 4096

#define EXIT_IF(_X) if ((_X) == 1) { exit(EXIT_FAILURE); }
#define RET_IF(_X) if ((_X) == 1) { return -1; }

#define PRINT_DEBUG(_X) printf("Var %s = %d\n", #_X, _X)

void resend_data(int fromfd, int tofd)
{
    struct buf_t* buf = buf_new(BUF_SIZE);
    EXIT_IF(buf == NULL);
    for (;;)
    {
        ssize_t readed = buf_fill(fromfd, buf, 1);
        EXIT_IF(readed == -1);
        if (readed == 0)
        {
            exit(EXIT_SUCCESS);
        }
        ssize_t written = buf_flush(tofd, buf, 1);
        EXIT_IF(written == -1);
    }
}

int bipipe(int cfd1, int cfd2)
{
    int result = 0;
    pid_t first_pid = 0;
    switch (first_pid = fork())
    {
        case -1:
            result = -1;
            goto bipipe_end;
        case 0:
            resend_data(cfd1, cfd2);
            break;
    }
    switch (fork())
    {
        case -1:
            kill(first_pid, SIGKILL);
            result = -1;
            goto bipipe_end;
        case 0:
            resend_data(cfd2, cfd1);
    }
bipipe_end:
    close(cfd1);
    close(cfd2);
    return result;
}

int listen_port(char* port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* tcp socket */

    s = getaddrinfo("localhost", port, &hints, &result);
    RET_IF(s != 0)

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
        {
            continue;
        }
        int yes = 1;
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            close(sfd);
            continue;
        }

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;                  
        }

        close(sfd);
    }
    RET_IF(rp == NULL)

    freeaddrinfo(result);

    RET_IF(listen(sfd, SOMAXCONN) == -1)
    return sfd;
}

int get_client(int sfd)
{
    struct sockaddr_storage client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sfd, (struct sockaddr*)&client, &client_len);
    if (cfd == -1 && 
            (errno == EAGAIN || errno == EINTR))
    {
        return 0;
    }
    return cfd;
}

void sig_handler_forking(int signo)
{
    if (signo == SIGCHLD)
    {
        wait(NULL);
        return;
    }
}

void set_sig_handler()
{
    struct sigaction new_action;
    new_action.sa_handler = sig_handler_forking;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        dprintf(STDERR_FILENO, "Usage: port filename\n");
        exit(EXIT_FAILURE);
    }
    set_sig_handler();
    char* const port1 = argv[1];
    char *const port2 = argv[2];
    int sfd1 = listen_port(port1);
    int sfd2 = listen_port(port2);
    EXIT_IF(sfd1 == -1)
    EXIT_IF(sfd2 == -1)

    for (;;) {
        int cfd1 = get_client(sfd1);
        if (cfd1 == 0) continue;
        int cfd2 = get_client(sfd2);
        while (cfd2 == 0)
        {
            cfd2 = get_client(sfd2);
        }
        if (cfd1 == -1 || cfd2 == -1)
        {
            close(sfd1);
            close(sfd2);
            exit(EXIT_FAILURE);
        }
        bipipe(cfd1, cfd2);
    }
}


