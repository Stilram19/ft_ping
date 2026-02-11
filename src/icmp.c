// ICMP packet building, parsing replies

#include "icmp.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>

// @brief calculates the checksum of the ICMP ECHO message 
// @param request is expected to have all fields set and checksum field is zero

static uint16_t calculateChecksum(icmp_echo_request_t *request) {
    uint32_t sum = 0;
 
    // Checksum header
    uint8_t *bytes = (uint8_t *)&(request->header);
    for (size_t i = 0; i < sizeof(icmp_echo_header_t); i += 2) {
        uint16_t word = (bytes[i] << 8) + bytes[i + 1];
        sum += word;
    }
    
    // Checksum data
    if (request->data && request->data_len) {
        bytes = request->data;
        for (size_t i = 0; i < request->data_len; i += 2) {
            uint16_t word;
            if (i + 1 < request->data_len) {
                word = (bytes[i] << 8) + bytes[i + 1];
            } else {
                word = bytes[i] << 8;  // odd byte handling
            }
            sum += word;
        }
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // return negation of the sum (1's complement)
    return (~sum);
}


int createIcmpEchoRequestMessage(ping_state_t *ping_state) {
    if (!ping_state) {
        return (ICMP_ERROR);
    }

    // (*) header
    ping_state->packet.header.type = 8;
    ping_state->packet.header.code = 0;
    ping_state->packet.header.identifier = htons(ping_state->identifier); // overwritten by the kernel in case of SOCK_DGRAM socket (user lacking privileges)
    ping_state->packet.header.sequence = htons(ping_state->sequence);
    ping_state->packet.header.checksum = 0;  // Will be calculated

    // (*) data
    if (ping_state->packet.data && ping_state->packet.data_len) {
        size_t off = 0;
        size_t len = ping_state->packet.data_len;
        if (ping_state->packet.data_len >= sizeof(struct timeval)) {
            // timestamp
            len -= sizeof(struct timeval);
            off += sizeof(struct timeval);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            tv.tv_sec = htonl(tv.tv_sec);
            tv.tv_usec = htonl(tv.tv_usec);
            memcpy(ping_state->packet.data, &tv, sizeof(tv));
        }

        // filling remaining data with zeros
        for (size_t i = 0; i < len; i++) {
            ping_state->packet.data[i + off] = 0;
        }
    }

    // (*) checksum

    // now we're ready to calculate the checksum
    ping_state->packet.header.checksum = calculateChecksum(&ping_state->packet); 

    return (ICMP_OK);
}
