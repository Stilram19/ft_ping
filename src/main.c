# include "parsing.h"
# include "macros.h"
# include "socket.h"
# include "utils.h"
# include <errno.h>
# include <netinet/in.h>
# include <string.h>

int main(int argc, char **argv) {
    if (argc == 1) {
        errorLogger("usage error: Destination address required", PING_ERROR);
    }

    // parsing input
    char *input = argv[1];
    char *display_addr = NULL;
    struct in_addr addr;

    if (parse_input_address(input, &addr, &display_addr) == PARSE_ERROR) {
        errorLogger(ft_strjoin(input, ": Name or service not known"), PING_ERROR);
    }

    debugLogger(display_addr);

    // creating raw ICMP socket
    int sock_fd;

    if (createRawICMPSocket(&sock_fd) == SOCKET_ERROR) {
        errorLogger(ft_strjoin("socket: ", strerror(errno)), PING_ERROR);
    }

    return (0);
}
