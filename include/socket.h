#ifndef SOCKET_H
# define SOCKET_H

// @brief creates a raw ICMP socket
// *sock_fd is set to the fd of the created socket in case of success
// @return SOCKET_ERROR to indicate error, SOCKET_OK otherwise
int createRawICMPSocket(int *sock_fd);

#endif
