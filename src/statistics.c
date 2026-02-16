#include "statistics.h"
#include "ft_ping.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

extern ping_state_t state;

void print_statistics(void) {
    unsigned long packet_loss = 0;

    if (state.num_sent) {
        packet_loss = ((state.num_sent - state.num_recv) * 100) / state.num_sent;
    }

    printf("--- %s ping statistics ---\n", state.hostname);
    printf("%lu packets transmitted, %lu packets received, %lu%% packet loss\n", state.num_sent, state.num_recv, packet_loss);
    
    // we calculate and print rtt stats only if we have received packets
    if (state.num_recv > 0) {
        double avg_sec = state.rrt_sum / state.num_recv;
        double variance = (state.rrt_sum_sq / state.num_recv) - (avg_sec * avg_sec);
        double variance_clamped = fmax(0.0, variance);
        double stddev_sec = sqrt(variance_clamped);

        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", 
               state.rrt_min * 1000.0, avg_sec * 1000.0, state.rrt_max * 1000.0, stddev_sec * 1000.0);
    }
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        print_statistics();
        exit(EXIT_SUCCESS);
    }
}
