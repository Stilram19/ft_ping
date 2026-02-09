#ifndef PARSING_H
# define PARSING_H

#include <netinet/in.h>

// @brief parses the input IPv4 (address / hostname);
// * initializes the struct pointed to by 'addr'
// * initializes the display_address (the clean numbers and dots IPv4 address)
// @return PARSE_ERROR if input is neither an IPv4 address nor a hostname
int parse_input_address(const char *input, struct in_addr *addr, char **display_address);

#endif
