#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/in.h>

// ICMP echo header structure
typedef struct {
    uint8_t  type;           // 8
    uint8_t  code;           // 0
    uint16_t checksum;       // will be calculated
    uint16_t identifier;     // process ID
    uint16_t sequence;       // incrementing sequence number
} icmp_echo_header_t;

// ICMP echo request structure
typedef struct {
    icmp_echo_header_t header;
    uint8_t *data;           // pointer to data
    size_t data_len;         // data length
} icmp_echo_request_t;



typedef struct ping_state {
    uint16_t identifier;
    uint16_t sequence;
    int sock_fd;
    char *display_address; // parsed IPv4 address (clean)
 
    // network addressing
    struct sockaddr_in dest_addr;       // destination socket address
    char *hostname;                     // hostname/IPv4 address input
 
    // packet structure & pre-allocated and sized
    icmp_echo_request_t packet;        // contains header + data pointer + data length
    size_t packet_size;                // Total packet size (constant = sizeof(icmp_echo_header_t) + packet.data_len)

    // Socket configuration
    int socket_type;                   // SOCK_RAW or SOCK_DGRAM
    int useless_identifier;            // 1 if kernel overrides ICMP ID, 0 otherwise (1 if the created socket's type is SOCK_DGRAM, 0 if it's SOCK_RAW)

    // statistics tracking
    unsigned long num_sent;            // packets sent
    unsigned long num_recv;            // packets received
    unsigned long num_rept;            // duplicate packets

    // runtime control
    size_t count;                      // number of packets to send (0 = infinite)

    // unsigned int interval;          // interval between packets (milliseconds)
    int timeout;                       // reply timeout (in seconds)

} ping_state_t;


// @brief ping loop
void start_pinging(ping_state_t *state);

#endif
