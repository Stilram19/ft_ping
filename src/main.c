#include "parsing.h"
#include "macros.h"
#include "socket.h"
#include "ft_ping.h"
#include "statistics.h"
#include "utils.h"
#include <float.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

ping_state_t state;

int main(int argc, char **argv) {
    state.program_name = argv[0];
    if (argc == 1) {
        errorLogger("missing host operand\nTry './ft_ping -h' for more information.", EX_USAGE);
    }

    signal(SIGINT, signal_handler);

    // (*) input parsing

    // global state initialization
    state.program_name = argv[0];
    state.identifier = getpid() & 0xFFFF;
    state.sequence = 0;     // will increment for each packet
    state.count = 0;
    state.verbose = 0;
    state.quiet = 0;
    state.wait = DEFAULT_PING_WAIT;
    state.flood = 0;
    state.num_recv = 0;
    state.num_sent = 0;
    state.num_rept = 0;
    state.packet.data_len = DEFAULT_DATALEN;
    state.rrt_max = 0.0;
    state.rrt_min = DBL_MAX;
    state.rrt_sum_sq = 0.0;
    state.rrt_sum = 0.0;

    char display_addr[MAX_IPV4_ADDR_LEN + 1] = {};

    int host_index = parse_options(argc, argv);

    if (host_index >= argc) {
        errorLogger("unknown host", EXIT_FAILURE);
    }

    struct in_addr addr;

    char *input = argv[host_index];
    if (parse_input_address(input, &addr, display_addr) == PARSE_ERROR) {
        errorLogger("unknown host", EXIT_FAILURE);
    }

    // debugLogger(display_addr);


    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = 0;
    saddr.sin_addr = addr;
    state.hostname = input;
    state.display_address = display_addr;
    state.dest_addr = saddr;

    // (*) raw ICMP socket creation
    int sock_fd;
    int sock_type;

    if (createPingSocket(&sock_fd, &sock_type, argv[0]) == SOCKET_ERROR) {
        int saved_errno = errno;
        const char *err_msg = (saved_errno != 0) ? strerror(saved_errno) : "socket creation error";
        errorLogger(ft_strjoin("socket: ", err_msg), EXIT_FAILURE);
    }

    // (*) initialize ping state
    uint8_t received[MAX_SEQUENCE + 1] = {0};

    state.sock_fd = sock_fd;
    state.socket_type = sock_type;
    state.useless_identifier = (sock_type == SOCK_DGRAM); // if the created socket's type is SOCK_DGRAM then the kernel will override the ICMP ID (hence useless_identifier)
    state.received = received;

    state.packet.data = NULL;

    if (state.packet.data_len) {
        state.packet.data = malloc(state.packet.data_len);

        if (!state.packet.data) {
            errorLogger(strerror(errno), EXIT_FAILURE);
        }
    }

    // (*) start pinging (ping loop)
    start_pinging();

    // (*) raw ICMP socket closing
    if (closePingSocket(sock_fd) == SOCKET_ERROR) {
        errorLogger(ft_strjoin("socket: ", strerror(errno)), EXIT_FAILURE);
    }
 
    return (0);
}
