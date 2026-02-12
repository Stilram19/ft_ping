#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include "ft_ping.h"

// @brief creates an ICMP socket: socket type is SOCK_RAW if the user is privileged (or the CAP_NET_RAW capability is set for the binary executable),
// the type is SOCK_DGRAM otherwise (a fallback to enable less privileged users to use ping utility)
// @param *sock_fd is set to the fd of the created socket in case of success
// @param *type is set to the type the created socket (in case of success): either SOCK_RAW or SOCK_DGRAM
// @return SOCKET_ERROR to indicate error, SOCKET_OK otherwise
int createPingSocket(int *sock_fd, int *type, char *program_name);

// @brief sends an ICMP Echo Request message to the destination address (a field in state)
int sendIcmpEchoMessage(ping_state_t *state);

// @brief closes sock_fd
// @return SOCKET_ERROR to indicate error, SOCKET_OK otherwise
int closePingSocket(int sock_fd);

// @brief waits until a message is received, it is then read into buffer, the sender's address is set into sender_addr param
// @return the number of bytes received in case of success, SOCKET_ERROR otherwise
ssize_t recvMessage(ping_state_t *state, void *buffer, size_t buffer_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len);

#endif
