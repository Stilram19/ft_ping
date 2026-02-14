#include "parsing.h"
#include "macros.h"
#include "socket.h"
#include "ft_ping.h"
#include "utils.h"
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc == 1) {
        errorLogger(argv[0], "missing host operand\nTry './ft_ping -h' for more information.", EX_USAGE);
    }

    // (*) input parsing
    int count = 0; // default (controlled by -c if specified)
    int timeout = NO_TIMEOUT; // default (controlled by -W if specified)
    int data_len = DEFAULT_DATALEN; // default (controlled by -s if specified)
    char *input = argv[1];
    char display_addr[MAX_IPV4_ADDR_LEN + 1] = {};

    struct in_addr addr;

    if (parse_input_address(input, &addr, display_addr) == PARSE_ERROR) {
        errorLogger(argv[0], ft_strjoin(input, ": Name or service not known"), PING_ERROR);
    }

    debugLogger(display_addr);


    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = 0;
    saddr.sin_addr = addr;

    // (*) raw ICMP socket creation
    int sock_fd;
    int sock_type;

    if (createPingSocket(&sock_fd, &sock_type, argv[0]) == SOCKET_ERROR) {
        int saved_errno = errno;
        const char *err_msg = (saved_errno != 0) ? strerror(saved_errno) : "socket creation error";
        errorLogger(argv[0], ft_strjoin("socket: ", err_msg), EXIT_FAILURE);
    }

    if (sock_type == SOCK_DGRAM) {
        debugLogger("socket type: SOCK_DGRAM");
    } else {
        debugLogger("socket type: SOCK_RAW");
    }

    // (*) initialize ping state
    uint8_t received[MAX_SEQUENCE + 1] = {0};
    struct ping_state state;

    state.program_name = argv[0];
    state.identifier = getpid() & 0xFFFF;
    state.sequence = 0;     // will increment for each packet
    state.sock_fd = sock_fd;
    state.socket_type = sock_type;
    state.useless_identifier = (sock_type == SOCK_DGRAM); // if the created socket's type is SOCK_DGRAM then the kernel will override the ICMP ID (hence useless_identifier)
    state.hostname = input;
    state.display_address = display_addr;
    state.dest_addr = saddr;
    state.count = count;
    state.verbose = 0;
    state.timeout = timeout;
    state.received = received;
    state.num_recv = 0;
    state.num_sent = 0;
    state.num_rept = 0;
    state.packet.data_len = data_len;
    state.packet.data = NULL;

    if (data_len) {
        state.packet.data = malloc(data_len);

        if (!state.packet.data) {
            errorLogger(argv[0], strerror(errno), EXIT_FAILURE);
        }
    }

    // (*) start pinging (ping loop)
    // start_pinging(&state);

    // (*) raw ICMP socket closing
    if (closePingSocket(sock_fd) == SOCKET_ERROR) {
        errorLogger(argv[0], ft_strjoin("socket: ", strerror(errno)), EXIT_FAILURE);
    }
 
    return (0);
}
