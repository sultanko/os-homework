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

void sig_handler_ignore(int signo)
{
    if (signo == SIGPIPE)
    {
        return;
    }
}

void set_sig_handler()
{
    struct sigaction new_action;
    new_action.sa_handler = sig_handler_ignore;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGPIPE, &new_action, NULL);
}

typedef struct {
    struct buf_t* buf[2];
    int can_read[2];
} pair_buffers_t;

#define MAX_FDS_SIZE 256
struct pollfd fds[MAX_FDS_SIZE];
pair_buffers_t buffs[127];
int fds_size;

void add_clients(int cfd1, int cfd2)
{
    buffs[fds_size/2 - 1].buf[0] = buf_new(BUF_SIZE);
    buffs[fds_size/2 - 1].buf[1] = buf_new(BUF_SIZE);
    buffs[fds_size/2 - 1].can_read[0] = 1;
    buffs[fds_size/2 - 1].can_read[1] = 1;
    memset(&fds[fds_size], 0, sizeof(struct pollfd));
    fds[fds_size].fd = cfd1;
    fds[fds_size].events = POLLIN;
    ++fds_size;
    memset(&fds[fds_size], 0, sizeof(struct pollfd));
    fds[fds_size].fd = cfd2;
    fds[fds_size].events = POLLIN;
    ++fds_size;
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

void try_read(int i)
{
    if ((fds[i].revents & POLLIN) && buffs[(i>>1) - 1].can_read[i&1] == 1)
    {
        // printf("POLLIN on %d\n", fds[i].fd);
        int index = (i>>1) - 1;
        struct buf_t* buf = buffs[index].buf[i&1];
        if (buf == NULL) { return; }
        int old_size = buf_size(buf);
        if (buf_fill(fds[i].fd, buf, old_size + 1) <= old_size) 
        { 
            shutdown(fds[i].fd, SHUT_RD);
            fds[i].events &= ~POLLIN;
            buffs[index].can_read[i&1] = 0;
            if (buf_size(buf) == 0)
            {
                buf_free(buf);
                buffs[index].buf[i&1] = NULL;
            }
            return;
        }
        if (buf_size(buf) == buf_capacity(buf))
        {
            fds[i].events &= ~POLLIN;
        }
        if (buf_size(buf) > 0)
        {
            fds[i^1].events |= POLLOUT;
        }
    }
}

void try_write(int i)
{
    if (fds[i].revents & POLLOUT)
    {
        // printf("POLLOUT on %d\n", fds[i].fd);
        int index = (i>>1) - 1;
        struct buf_t* buf = buffs[index].buf[(i&1)^1];
        if (buf == NULL) { return; }
        int flush_res = buf_flush(fds[i].fd, buf, 1);
        // PRINT_DEBUG(flush_res);
        if (flush_res == -1) 
        { 
            shutdown(fds[i].fd, SHUT_WR);
            shutdown(fds[i^1].fd, SHUT_RD);
            fds[i].events &= ~POLLOUT;
            fds[i^1].events &= ~POLLIN;
            buf_free(buf);
            buffs[index].buf[(i&1)^1] = NULL;
            buffs[index].can_read[(i&1)^1] = 0;
            return;
        }
        if (buf_size(buf) == 0)
        {
            fds[i].events &= ~POLLOUT;
            if (buffs[index].can_read[(i&1)^1] == 0)
            {
                buf_free(buf);
                buffs[index].buf[(i&1)^1] = NULL;
            }
        }
        if (buf_size(buf) < buf_capacity(buf) && buffs[(i>>1) - 1].can_read[(i&1)^1] == 1)
        {
            fds[i^1].events |= POLLIN;
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        // dprintf(STDERR_FILENO, "Usage: port1 por2\n");
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
        int count = poll(fds, fds_size, -1);
        if (count == 0)
        {
            continue;
        }
        if (count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }
        if (fds_size < MAX_FDS_SIZE && (fds[state].revents & POLLIN))
        {
            int client_fd = get_client(fds[state].fd);
            if (state == 0)
            {
                cfd1 = client_fd;
            }
            else
            {
                // printf("Accepted %d %d\n", cfd1, client_fd);
                add_clients(cfd1, client_fd);
            }
            state = state ^ 1;
        }
        for (int i = 2; i < fds_size; ++i)
        {
            if ((fds[i].revents & POLLHUP)
                    || (fds[i].revents & POLLERR))
            {
                // if error occur on one of socket pair
                // close both sockets
                // printf("POLLHUP on %d\n", fds[i].fd);
                shutdown(abs(fds[i].fd), SHUT_RDWR);
                int index = (i>>1) - 1;
                if (buffs[index].buf[(i&1)^1] != NULL) { buf_free(buffs[index].buf[(i&1)^1]);} 
                buffs[index].buf[(i&1)^1] = NULL;
                buffs[index].can_read[i&1] = 0;
                fds[i].fd = -fds[i].fd;
                continue;
            }
            try_read(i);
            try_write(i);
        }
        for (int i = 2; i < fds_size; i+=2)
        {
            int buffs_index = (i>>1) - 1;
            // printf("Read status %d %d\n", buffs[buffs_index].can_read[0], buffs[buffs_index].can_read[1]);
            if ((buffs[buffs_index].can_read[0] == 0 
                    && buffs[buffs_index].can_read[1] == 0
                    && buffs[buffs_index].buf[0] == NULL
                    && buffs[buffs_index].buf[1] == NULL)
                    || fds[i].fd < 0
                    || fds[i^1].fd < 0
               )
            {
                close(abs(fds[i].fd));
                close(abs(fds[i+1].fd));
                if (buffs[buffs_index].buf[0] != NULL) { buf_free(buffs[buffs_index].buf[0]);} 
                if (buffs[buffs_index].buf[1] != NULL) { buf_free(buffs[buffs_index].buf[1]);} 
                fds[i] = fds[fds_size - 2];
                fds[i+1] = fds[fds_size - 1];
                fds_size -= 2;
            }
        }
    }
}


