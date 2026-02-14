#ifndef PARSING_H
#define PARSING_H

#include "ft_ping.h"
#include <netinet/in.h>

// @brief parses the input IPv4 (address / hostname);
// * initializes the struct pointed to by 'addr'
// * copies the parsed input address into the pointer pointed to by display_address
// * '*display_address' must point an allocated block of memory with at least (MAX_IPV4_ADDR_LEN + 1) bytes
// @return PARSE_ERROR if input is neither an IPv4 address nor a hostname
int parse_input_address(const char *input, struct in_addr *addr, char *display_address);

// @brief parses supported options
// return exits in case of error, otherwise PARSE_OK
int parse_options(int argc, char **argv, ping_state_t *state);

#endif
