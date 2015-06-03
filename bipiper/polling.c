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
#include <poll.h>

#define BUF_SIZE 4096

#define EXIT_IF(_X) if ((_X) == 1) { exit(EXIT_FAILURE); }
#define RET_IF(_X) if ((_X) == 1) { return -1; }

#define PRINT_DEBUG(_X) printf("Var %s = %d\n", #_X, _X)

int listen_port(char* port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */

    s = getaddrinfo("localhost", port, &hints, &result);
    RET_IF(s != 0)

   /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1)
        {
            continue;
        }
        int yes = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break;                  
        }

        close(sfd);
    }
    RET_IF(rp == NULL)

    freeaddrinfo(result);           /* No longer needed */

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

int make_non_blocking(int fd)
{
    int opts = fcntl(fd, F_GETFL, 0);
    RET_IF(fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0);
    return 0;
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

typedef struct {
    struct buf_t* buf[2];
} pair_buffers_t;

struct pollfd fds[256];
pair_buffers_t buffs[127];
int fds_size;

void add_clients(int cfd1, int cfd2)
{
    buffs[fds_size/2 - 1].buf[0] = buf_new(BUF_SIZE);
    buffs[fds_size/2 - 1].buf[1] = buf_new(BUF_SIZE);
    fds[fds_size].fd = cfd1;
    fds[fds_size].events = POLLIN | POLLHUP;
    ++fds_size;
    fds[fds_size].fd = cfd2;
    fds[fds_size].events = POLLIN | POLLHUP;
    ++fds_size;
}

int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        dprintf(STDERR_FILENO, "Usage: port1 por2\n");
        exit(EXIT_FAILURE);
    }
    set_sig_handler();
    char* const port1 = argv[1];
    char *const port2 = argv[2];
    fds[0].fd = listen_port(port1);
    EXIT_IF(fds[0].fd == -1);
    fds[0].events = POLLIN;
    make_non_blocking(fds[0].fd);
    fds[1].fd = listen_port(port2);
    EXIT_IF(fds[1].fd == -1);
    fds[1].events = POLLIN;
    make_non_blocking(fds[1].fd);
    fds_size = 2;

    int state = 0;
    int cfd1 = -1;
    for (;;) {
        int count = poll(fds, fds_size, 2000);
        if (count == 0)
        {
            continue;
        }
        if (count == -1)
        {
            break;
        }
        if (fds[state].revents & POLLIN)
        {
            int client_fd = get_client(fds[state].fd);
            if (state == 0)
            {
                cfd1 = client_fd;
            }
            else
            {
                add_clients(cfd1, client_fd);
            }
            state = state ^ 1;
        }
        for (int i = 2; i < fds_size; ++i)
        {
            if ((fds[i].revents & POLLHUP)
                    || (fds[i].revents & POLLERR))
            {
closing_client_pair:
                close(fds[i].fd);
                close(fds[i^1].fd);
                fds[i].fd = -fds[i].fd;
                fds[i^1].fd = -fds[i^1].fd;
                if ((i&1) == 0)
                {
                    ++i;
                }
                continue;
            }
            if (fds[i].revents & POLLIN)
            {
                struct buf_t* buf = buffs[i/2 - 1].buf[i&1];
                int old_size = buf_size(buf);
                if (buf_fill(fds[i].fd, buf, old_size + 1) <= old_size) { goto closing_client_pair; }
                if (buf_size(buf) == buf_capacity(buf))
                {
                    fds[i].events ^= POLLIN;
                }
                if (buf_size(buf) > 0)
                {
                    fds[i^1].events |= POLLOUT;
                }
            }
            if (fds[i].revents & POLLOUT)
            {
                struct buf_t* buf = buffs[i/2 - 1].buf[(i&1)^1];
                if (buf_flush(fds[i].fd, buf, 1) == -1) { goto closing_client_pair; }
                if (buf_size(buf) == 0)
                {
                    fds[i].events ^= POLLOUT;
                }
            }
        }
        for (int i = 2; i < fds_size; i+=2)
        {
            if (fds[i].fd < 0)
            {
                fds[i] = fds[fds_size - 2];
                fds[i+1] = fds[fds_size - 1];
                fds_size -= 2;
            }
        }
    }
}


