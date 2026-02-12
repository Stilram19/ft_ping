#define _DEFAULT_SOURCE
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "macros.h"
#include "parsing.h"

int parse_input_address(const char *input, struct in_addr *addr, char *display_address) {
    if (!input || !addr || !display_address) {
        return (PARSE_ERROR);
    }

    if (inet_aton(input, addr) != 0) {
        const char *parsed_address = inet_ntoa(*addr);
        size_t parsed_address_len = strlen(parsed_address);
        strncpy(display_address, parsed_address, parsed_address_len); // inet_ntoa returns the underlying static buffer that is overwritten with each subsequent call
        display_address[parsed_address_len] = '\0';
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
        size_t parsed_address_len = strlen(parsed_address);
        strncpy(display_address, parsed_address, parsed_address_len); // inet_ntoa returns the underlying static buffer that is overwritten with each subsequent call
        display_address[parsed_address_len] = '\0';
        freeaddrinfo(res);
        return (PARSE_OK);
    }

    return (PARSE_ERROR);
}
