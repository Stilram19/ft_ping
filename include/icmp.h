#ifndef ICMP_H
#define ICMP_H

#include "ft_ping.h"

// for function return status (general operations)
typedef enum {
    ICMP_OK,       // Operation successful
    ICMP_ERROR,    // Operation failed (malloc, socket error, etc.)
} icmp_status_t;

// for parsing result status (parsing packets)
typedef enum {
    PARSE_OK,           // valid ICMP response (echo reply or error message)
    PARSE_NETWORK_NOISE, // valid packet but not for us (or corrupt)
    PARSE_ERROR,        // failed to parse the packet
} parse_status_t;

// @brief fills the given request with an ICMP packet 
// @param ping_state takes the ping_state structure where it can find the current sequence, identifier... (useful info when constructing the ICMP header)
// * this function is not responsible of releasing any resources (memory...)
// @return returns ICMP_ERROR in case of error, ICMP_OK otherwise
int createIcmpEchoRequestMessage(ping_state_t *ping_state);

// @brief parses the incoming ICMP message and it either calls the handler of the ICMP message (or type of messages) or ignores the packet
// @return returns NETWORK_NOISE in case of network noise (the received packet is to be ignored), ICMP_ERROR to indicate error, ICMP_OK if the ICMP message 
// was parsed and the result was logged successfully
int parseIcmpMessageAndLogResult(ping_state_t *state, void *packet, size_t packet_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len);

#endif
