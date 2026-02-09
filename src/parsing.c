# define _DEFAULT_SOURCE
# include <string.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# include "macros.h"

int parse_input_address(const char *input, struct in_addr *addr, char **display_address) {
    if (!input || !addr || !display_address) {
        return (PARSE_ERROR);
    }

    if (inet_aton(input, addr) != 0) {
        const char *parsed_address = inet_ntoa(*addr);
        strcpy(*display_address, parsed_address); // inet_ntoa returns the underlying static buffer that is overwritten with each subsequent call
        return (PARSE_OK);
    }

    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(input, NULL, &hints, &res) == 0) {
        struct sockaddr_in *sin = (struct sockaddr_in *)(res->ai_addr);
        *addr = sin->sin_addr;
        const char *parsed_address = inet_ntoa(*addr);
        strcpy(*display_address, parsed_address); // inet_ntoa returns the underlying static buffer that is overwritten with each subsequent call
        freeaddrinfo(res);
        return (PARSE_OK);
    }

    return (PARSE_ERROR);
}
