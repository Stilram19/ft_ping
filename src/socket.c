// socket creation, send/recv logic

#include <netinet/in.h>
#include <sys/socket.h>
# include "macros.h"

int createRawICMPSocket(int *sock_fd) {
    if (!sock_fd) {
        return (SOCKET_ERROR);
    }

    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (fd == -1) {
        return (SOCKET_ERROR);
    }

    *sock_fd = fd;
    return (SOCKET_OK);
}
