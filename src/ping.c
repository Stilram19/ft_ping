#include "ft_ping.h"
#include "utils.h"
#include "socket.h"
#include "icmp.h"
#include "macros.h"
#include <errno.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>


static void waitBetweenPings(ping_state_t *state) {
    if (!state) {
        debugLogger("waitBetweenPings: state pointer is NULL");
        return;
    }
    struct timespec ts;
    ts.tv_sec = (time_t)state->wait;
    ts.tv_nsec = (long)((state->wait - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

static void first_ping_log(ping_state_t *state) {
    printf("PING %s (%s): %zu data bytes", state->hostname, state->display_address, state->packet.data_len);
    // handle verbose on
    printf("\n");
}

void start_pinging(ping_state_t *state) {
    if (!state) {
        debugLogger("start_pinging: state pointer is NULL");
        return;
    }

    first_ping_log(state);

    size_t count = state->count;
    int isLoopInfinite = (count == 0); // in inetutils-2.0 implementation (they consider -c 0 as loop infinitely)
    uint8_t buffer[PING_MAX_PACKET_SIZE];
    struct sockaddr sender_addr = {0};
    socklen_t sender_addr_len = {0};
    int received = 1;

    while (count || isLoopInfinite) {
        // create ICMP ECHO request message
        if (received == 1) {
            if (createIcmpEchoRequestMessage(state) == ICMP_ERROR) {
                infoLogger(state->program_name, "Error while creating ICMP echo request message");
                waitBetweenPings(state); // wait
                continue;
            }

            // send ICMP ECHO request message to destination
            if (sendIcmpEchoMessage(state) == SOCKET_ERROR) {
                infoLogger(state->program_name, "Error while sending ICMP echo request");
                waitBetweenPings(state); // wait
                continue;
            }

            // mark ICMP ECHO as sent
            if (!isLoopInfinite && count > 0) {
                count -= 1;
            }
        }

        // mark ICMP message as not yet received
        received = 0;

        // receiving network packet 
        ssize_t packet_len = recvMessage(state, buffer, PING_MAX_PACKET_SIZE, &sender_addr, &sender_addr_len);
        if (packet_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                infoLogger(state->program_name, "Timeout: no reply");
            } else {
                infoLogger(state->program_name, "Error while receiving network packet");
            }
            
            if (count || isLoopInfinite) {
                waitBetweenPings(state); // wait
            }
            received = 1;
            continue;
        }

        // parse network packet and log result or ignore if the packet is network noise 
        int parseStatus = parseIcmpMessageAndLogResult(state, buffer, packet_len, &sender_addr, &sender_addr_len);

        if (parseStatus == ICMP_OK) {
            received = 1; // mark it as received, otherwise we've only received NETWORK_NOISE, or some error occured
        }

        // wait between each send (only sleep if the reply was sent)
        if (received && (count || isLoopInfinite)) {
            waitBetweenPings(state);
        }
    }
}
