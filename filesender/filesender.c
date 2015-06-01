#include <stdlib.h>
#include <unistd.h>
#include <bufio.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 4096

#define FERROR(_X) if ((_X) == -1) { exit(EXIT_FAILURE); }

void file_send(int sfd, int cfd, char* filename, struct sockaddr_storage* client)
{
    int file_fd = open(filename, O_RDONLY);
    struct buf_t* buf = buf_new(BUF_SIZE);
    ssize_t buf_size = -1;
    for (;;)
    {
        buf_size = buf_fill(file_fd, buf, 1);
        if (buf_size == 0 || buf_size == -1)
        {
            break;
        }
        buf_flush(cfd, buf, buf_size);
    }
    buf_free(buf);
    close(file_fd);
    close(cfd);
}


int main(int argc, char* argv[]) {
    if (argc != 3)
    {
        dprintf(STDERR_FILENO, "Usage: port filename\n");
        exit(EXIT_FAILURE);
    }
    char* const port = argv[1];
    char *const filename = argv[2];
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        exit(EXIT_FAILURE);
    }

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

       if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
       {
            break;                  
       }

       close(sfd);
    }


   if (rp == NULL) {               /* No address succeeded */
        exit(EXIT_FAILURE);
   }

   FERROR(listen(sfd, -1));

   freeaddrinfo(result);           /* No longer needed */

    struct sockaddr_storage client;
    socklen_t client_len = sizeof(client);
    for (;;) {
        int cfd = accept(sfd, (struct sockaddr*)&client, &client_len);
        if (cfd == -1)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            close(sfd);
            exit(EXIT_FAILURE);
        }
        pid_t childPid = -1;
        switch (childPid = fork())
        {
            case -1:
                break;
            case 0:
                file_send(sfd, cfd, filename, &client);
                break;
            default:
                close(cfd);
                break;
        }
    }
}


