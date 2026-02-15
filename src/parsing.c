#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <sysexits.h>
#include <stdlib.h>
#include "ft_ping.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "macros.h"
#include "parsing.h"

static int is_all_digits(const char *str) {
    if (!str || *str == '\0')
        return (0);
    while (*str) {
        if (!isdigit((unsigned char)*str))
            return (0);
        str++;
    }
    return (1);
}

static void display_version() {
    printf("ft_ping (GNU inetutils) 2.0");
}

static void print_help(const char *program_name) {
    printf("Usage: %s [options] <hostname/IP>\n\n", program_name);
    printf("Options:\n");
    printf("  -c <count>    Stop after sending <count> packets\n");
    printf("  -s <size>     Packet data size (bytes)\n");
    printf("  -v            Verbose output\n");
    printf("  -q            Quiet mode\n");
    printf("  -f            Flood ping (send as fast as possible)\n");
    printf("  -i <number>   wait number seconds between sending each packet");
    printf("  -h, -?        Show this help message\n");
}

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

int parse_options(int argc, char **argv, ping_state_t *state) {
    if (!argv || !state) {
        errorLogger("./ft_ping", "parse_options: argument pointer cannot be NULL", EXIT_FAILURE);
    }

    int opt_index = 1;

    while (opt_index < argc && argv[opt_index][0] == '-') {
        char *arg = argv[opt_index];

        if (strcmp(arg, "-s") == 0) {
            // Check if there's a next argument
            if (opt_index + 1 >= argc) {
                errorLogger(argv[0], "-s: option requires an argument\nTry 'ping -h' or 'ping -?' for more information.", EX_USAGE);
            }
 
            char *value_str = argv[++opt_index];
            
            // check if it's all digits
            if (!is_all_digits(value_str)) {
                errorLogger(argv[0], "-s: invalid packet size", EX_USAGE);
            }
 
            int value = atoi(value_str);
 
            // Check if it's in valid range (inetutils allows 0-65507 for IPv4)
            if (value < 0 || value > MAX_DATALEN_OPTION) {
                errorLogger(argv[0], "-s: value too large", EX_USAGE);
            }
 
            state->packet.data_len = value;
        } else if (strcmp(arg, "-c") == 0) {
            // check if there's a next argument
            if (opt_index + 1 >= argc) {
                errorLogger(argv[0], "-c: option requires an argument\nTry 'ping -h' or 'ping -?' for more information.", EX_USAGE);
            }
            
            char *value_str = argv[++opt_index];
            char *endptr;
            
            // check if it's all digits
            if (!is_all_digits(value_str)) {
                errorLogger(argv[0], "-c: invalid count", EX_USAGE);
            }
 
            long value = strtol(value_str, &endptr, 10);
 
            // check for overflow
            if (value < 0 || value > INT_MAX) {
                errorLogger(argv[0], "-c: value too large", EX_USAGE);
            }
 
            state->count = (int)value;
        } else if (strcmp(arg, "-i") == 0) {
            // check if there's a next argument
            if (opt_index + 1 >= argc) {
                errorLogger(argv[0], "-i: option requires an argument", EX_USAGE);
            }

            char *value_str = argv[++opt_index];
            char *endptr;

            // parse as float
            float value = strtof(value_str, &endptr);
            
            // check if conversion was successful (endptr should be at end of string)
            if (endptr == value_str || *endptr != '\0') {
                errorLogger(argv[0], "-i: invalid interval", EX_USAGE);
            }

            // always reject negative or zero values
            if (value <= 0.0f) {
                errorLogger(argv[0], "-i: interval must be positive", EX_USAGE);
            }

            // check minimum interval based on privilege level
            float min_interval = (geteuid() == 0) ? 0.001 : 0.2f;
 
            if (value < min_interval) {
                if (geteuid() != 0) {
                    errorLogger(argv[0], "-i: interval must be at least 0.2", EX_USAGE);
                } else {
                    errorLogger(argv[0], "-i: interval must be at least 0.001", EX_USAGE);
                }
            }

            state->wait = value;
        } else if (strcmp(arg, "-v") == 0) {
            state->verbose = 1;
        } else if (strcmp(arg, "-V") == 0) {
            display_version();
        } else if (strcmp(arg, "-q") == 0) {
            state->verbose = 0;  // quiet mode (opposite of verbose)
            state->quiet = 1;
        } else if (strcmp(arg, "-f") == 0) {
            if (geteuid() != 0) {
                errorLogger(argv[0], "-f: only root can use flood ping", EX_NOPERM);
            }
            state->flood = 1;
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "-?") == 0) {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            errorLogger(argv[0], "unknown option", EX_USAGE);
        }
        opt_index += 1;
    }

    return (opt_index);
}
