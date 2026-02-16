#include "ft_ping.h"
#include "utils.h"
#include "socket.h"
#include "icmp.h"
#include "macros.h"
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// @param sleep wait in seconds
static void waitBetweenPings(float wait) {
    struct timespec ts;
    ts.tv_sec = (time_t)wait;
    ts.tv_nsec = (long)((wait - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

static void first_ping_log(ping_state_t *state) {
    printf("PING %s (%s): %zu data bytes", state->hostname, state->display_address, state->packet.data_len);

    if (state->verbose) {
        printf(", id 0x%04x = %d", state->identifier, state->identifier);
    }

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
    float wait_interval = (state->flood == 1) ? 0.01 : state->wait; // interval (in seconds) to wait between each two sends

    while (count || isLoopInfinite) {
        // create ICMP ECHO request message
        if (createIcmpEchoRequestMessage(state) == ICMP_ERROR) {
            infoLogger(state->program_name, "Error while creating ICMP echo request message");
            waitBetweenPings(state->wait); // wait
            continue;
        }

        // send ICMP ECHO request message to destination
        if (sendIcmpEchoMessage(state) == SOCKET_ERROR) {
            infoLogger(state->program_name, "Error while sending ICMP echo request");
            waitBetweenPings(state->wait); // wait
            continue;
        }

        // mark ICMP ECHO as sent
        if (!isLoopInfinite && count > 0) {
            count -= 1;
        }

        // when flood mode is on, log '.' after echo ECHO REQUEST message is sent
        if (state->quiet == 0 && state->flood == 1) {
            printf(".");
            fflush(stdout);
        }

        // (*) productive wait (using select & recvfrom)
        struct timespec start_time, current_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        while (1) {
            // calculate elapsed time since we sent the packet
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            double elapsed = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

            // if the wait interval is over, break the inner loop to send the next packet
            if (elapsed >= wait_interval || (!isLoopInfinite && state->num_recv == state->count)) {
                break;
            }

            struct timeval select_timeout;
            select_timeout.tv_sec = 0;
            select_timeout.tv_usec = 10000; // 10ms

            fd_set read_fds;

            FD_ZERO(&read_fds);
            FD_SET(state->sock_fd, &read_fds);

            int select_ret = select(state->sock_fd + 1, &read_fds, NULL, NULL, &select_timeout);

            if (select_ret > 0) {
                // drain socket receive buffer
                while (1) {
                    struct sockaddr_in sender_addr;
                    memset(&sender_addr, 0, sizeof(sender_addr));
                    socklen_t sender_addr_len = sizeof(sender_addr);
                    ssize_t packet_len = recvfrom(state->sock_fd, buffer, PING_MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *)&sender_addr, &sender_addr_len);
                    if (packet_len > 0) {
                        parseIcmpMessageAndLogResult(state, buffer, packet_len, (struct sockaddr *)&sender_addr, &sender_addr_len);
                    } else {
                        break;
                    }
                }
            } else if (state->quiet == 0 && state->flood == 0 && select_ret < 0 && errno != EINTR) {
                infoLogger(state->program_name, "Select() failed");
                break;
            }
        }
    }


    // if we haven't received enough ECHO REPLIES (compared to sent ECHO REQUESTS), we wait for 1 second (giving another last chance)
    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (state->num_recv < state->count) {
        // calculate elapsed time since we sent the packet
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        // if the wait interval (1 second) is over, break the inner loop to send the next packet
        if (elapsed >= DEFAULT_PING_WAIT) {
            break;
        }

        struct timeval select_timeout;
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 10000; // 10ms

        fd_set read_fds;

        FD_ZERO(&read_fds);
        FD_SET(state->sock_fd, &read_fds);

        int select_ret = select(state->sock_fd + 1, &read_fds, NULL, NULL, &select_timeout);

        if (select_ret > 0) {
            // drain socket receive buffer
            while (1) {
                struct sockaddr_in sender_addr;
                memset(&sender_addr, 0, sizeof(sender_addr));
                socklen_t sender_addr_len = sizeof(sender_addr);
                ssize_t packet_len = recvfrom(state->sock_fd, buffer, PING_MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr *)&sender_addr, &sender_addr_len);
                if (packet_len > 0) {
                    parseIcmpMessageAndLogResult(state, buffer, packet_len, (struct sockaddr *)&sender_addr, &sender_addr_len);
                } else {
                    break;
                }
            }
        } else if (state->quiet == 0 && state->flood == 0 && select_ret < 0 && errno != EINTR) {
            infoLogger(state->program_name, "Select() failed");
            break;
        }
    }

    if (state->quiet == 0 && state->flood == 1) {
        printf("\n");
    }

    printf("sent packets: %lu\n", state->num_sent);
    printf("received packets: %lu\n", state->num_recv);
}
