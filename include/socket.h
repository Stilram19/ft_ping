#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include "ft_ping.h"

// @brief creates a raw ICMP socket
// @param *sock_fd is set to the fd of the created socket in case of success
// @param *type is set to the type the created socket (in case of success): either SOCK_RAW or SOCK_DGRAM
// @return SOCKET_ERROR to indicate error, SOCKET_OK otherwise
int createRawIcmpSocket(int *sock_fd, int *type, char *program_name);

// @brief sends the ICMP message to the destination address (addr)
int sendIcmpEchoMessage(ping_state_t *state);

// @brief closes sock_fd (sock_fd should be a return value of createRawIcmpSocket)
// @return SOCKET_ERROR to indicate error, SOCKET_OK otherwise
int closeRawIcmpSocket(int sock_fd);

#endif
