# define _DEFAULT_SOURCE
# include <string.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# include "macros.h"

int parse_input_address(const char *input, struct in_addr *addr, char **display_address) {
    if (inet_aton(input, addr) != 0) {
        *display_address = inet_ntoa(*addr);
        return (PARSE_OK);
    }

    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP; // Internet Control Message Protocol

    if (getaddrinfo(input, NULL, &hints, &res) == 0) {
        struct sockaddr_in *sin = (struct sockaddr_in *)(res->ai_addr);
        *addr = sin->sin_addr;
        *display_address = inet_ntoa(*addr);
        freeaddrinfo(res);
        return (PARSE_OK);
    }

    return (PARSE_ERROR);
}
