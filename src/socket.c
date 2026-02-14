// socket creation, send/recv logic

#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include "ft_ping.h"
#include "macros.h"
#include "socket.h"
#include "utils.h"

int createPingSocket(int *sock_fd, int *sock_type, char *program_name) {
    if (!sock_fd || !sock_type || !program_name) {
        debugLogger("createPingSocket: args pointers cannot be NULL");
        return (SOCKET_ERROR);
    }

    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (fd < 0) {
        if (errno == EPERM || errno == EACCES) {
            // fallback to SOCK_DGRAM for unprivileged users (lacking CAP_NET_RAW capability)
            // this fallback may only work in linux 
            errno = 0;
            fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

            if (fd < 0) {
                if (errno == EPERM || errno == EACCES || errno == EPROTONOSUPPORT) {
                    errorLogger(program_name, "Lacking privilege for icmp socket.", EXIT_FAILURE);
                }
                return (SOCKET_ERROR);
            }

            *sock_fd = fd;
            *sock_type = SOCK_DGRAM;
            return (SOCKET_OK);
        }
        return (SOCKET_ERROR);
    }

    *sock_fd = fd;
    *sock_type = SOCK_RAW;
    return (SOCKET_OK);
}

int closePingSocket(int sock_fd) {
    if (sock_fd < 0) {
        return (SOCKET_ERROR);
    }
    
    if (close(sock_fd) == -1) {
        return (SOCKET_ERROR);
    }
    
    return (SOCKET_OK);
}

int sendIcmpEchoMessage(ping_state_t *state) { 
    if (!state) {
        debugLogger("sendIcmpEchoMessage: state pointer in NULL");
        return (SOCKET_ERROR);
    }

    // static buffer to avoid repeated heap alloc/free (as this function is called repetitively)
    static uint8_t packet_buffer[PING_MAX_PACKET_SIZE];

    // calculating total packet size
    size_t packet_size = sizeof(icmp_echo_header_t) + state->packet.data_len;
    
    if (packet_size > PING_MAX_PACKET_SIZE) {
        return (SOCKET_ERROR);  // Packet too large
    }
    
    // copy header to buffer
    memcpy(packet_buffer, &state->packet.header, sizeof(icmp_echo_header_t));
 
    // Copy data to buffer (if any)
    if (state->packet.data && state->packet.data_len) {
        memcpy(packet_buffer + sizeof(icmp_echo_header_t), 
               state->packet.data, state->packet.data_len);
    }
 
    ssize_t ret = sendto(state->sock_fd, packet_buffer, packet_size, 0, (struct sockaddr *)&(state->dest_addr), sizeof(struct sockaddr_in));
 
    if (ret < 0) {
        return (SOCKET_ERROR);
    }
    
    if ((size_t)ret != packet_size) {
        // partial send
        return (SOCKET_ERROR);
    }

    // clearing the received flag for the sequence of the packet just sent, marking it as not yet received
    state->received[state->sequence] = 0;

    // increment sequence and number of sent packets for the next transmission
    state->sequence += 1;
    state->num_sent += 1;

    return (SOCKET_OK);
}

ssize_t recvMessage(ping_state_t *state, void *buffer, size_t buffer_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len) {
    if (!state) {
        return (SOCKET_ERROR);
    }

    ssize_t ret = recvfrom(state->sock_fd, buffer, buffer_len, 0, sender_addr, sender_addr_len);

    if (ret < 0) {
        return (SOCKET_ERROR);
    }

    return (ret);
}
