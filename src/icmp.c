// ICMP packet building, parsing replies

#define _DEFAULT_SOURCE
#include "ft_ping.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "icmp.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <sys/socket.h>
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
    ping_state->packet.header.type = ICMP_ECHO;
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

    // now we're ready to calculate the checksum (all fields are already in network byte order, so checksum is automatically in network byte order)
    ping_state->packet.header.checksum = calculateChecksum(&ping_state->packet); 

    return (ICMP_OK);
}


void handle_echo_reply(ping_state_t *state, struct icmphdr *icmp, void *data, size_t data_len, size_t packet_len, uint8_t ttl) {
    if (!icmp || !state || !data) {
        debugLogger("handle_echo_reply: pointer args cannot be null");
        return;
    }

    // validating the identifier only if the socket type is SOCK_RAW (useless_identification is set to 0)
    if (state->useless_identifier == 0 && state->identifier != icmp->un.echo.id) {
        infoLogger(state->program_name, "handle_echo_reply: received packet has a different ID (to be ignored)");
        return; // ignore
    }
 
    int isDuplicate = (state->received[icmp->un.echo.sequence] == 1);

    // RRT (round-trip time)
    float rrt = -1;

    if (data_len >= sizeof(struct timeval)) {
        struct timeval echo_request_time = *(struct timeval*)data;
        struct timeval echo_reply_time;

        gettimeofday(&echo_reply_time, NULL);
        rrt = (echo_reply_time.tv_sec - ntohl(echo_request_time.tv_sec)) * 1000.0 + (echo_reply_time.tv_usec - ntohl(echo_request_time.tv_usec) / 1000000.0);
    }

    // printing result
    printf("%lu bytes from %s: icmp_seq=%u ttl=%u", packet_len, state->display_address, state->sequence, ttl);

    if (rrt > 0) {
        printf(" time=%f", rrt);
    }

    if (isDuplicate) {
        printf (" (DUP!)");
    }

    printf("\n");


    // current sequence is consumed
    state->received[state->sequence] = 1; // mark sequence as received
    state->sequence += 1;
}

void handle_error_message(struct sockaddr_in *saddr, icmphdr *icmp_header) {
    if (!saddr || !icmp_header) {
        debugLogger("handle_error_message: pointer args cannot be NULL");
        return;
    }

    printf("From %s", inet_ntoa(saddr->sin_addr));

    // destination unreachable message
    if (icmp_header->type == ICMP_DEST_UNREACH) {
        if (icmp_header->code == ICMP_NET_UNREACH) {
            printf(" Destination Net Unreachable\n");
            return;
        }
        if (icmp_header->code == ICMP_HOST_UNREACH) {
            printf(" Destination Host Unreachable\n");
            return;
        }
        if (icmp_header->code == ICMP_PROT_UNREACH) {
            printf(" Destination Protocol Unreachable\n");
            return;
        }
        if (icmp_header->code == ICMP_PORT_UNREACH) {
            printf(" Destination Port Unreachable\n");
            return;
        }
        if (icmp_header->code == ICMP_FRAG_NEEDED) {
            printf(" Fragmentation needed and DF set\n");
            return;
        }
        if (icmp_header->code == ICMP_SR_FAILED) {
            printf(" Source route failed\n");
            return;
        }
        return;
    }

    // time exceeded message
    if (icmp_header->type == ICMP_TIME_EXCEEDED) {
        if (icmp_header->code == ICMP_EXC_TTL) {
            printf(" TTL count exceeded\n");
            return;
        }
        if (icmp_header->code == ICMP_EXC_FRAGTIME) {
            printf(" Fragment Reass time exceeded\n");
            return;
        }
        return;
    }

    // redirect message
    if (icmp_header->type == ICMP_REDIRECT) {
        if (icmp_header->code == ICMP_REDIR_NET) {
            printf(" Redirect Net\n");
            return;
        }

        if (icmp_header->code == ICMP_REDIR_HOST) {
            printf(" Redirect Host\n");
            return;
        }

        if (icmp_header->code == ICMP_REDIR_NETTOS) {
            printf(" Redirect Net for TOS\n");
            return;
        }

        if (icmp_header->code == ICMP_REDIR_HOSTTOS) {
            printf(" Redirect Host for TOS\n");
            return;
        }
        return;
    }
}

void parseIcmpMessage(ping_state_t *state, void *packet, size_t packet_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len) {
    // debugging guard
    if (!state) {
        debugLogger("parseIcmpMessage: Fatal error: state struct pointer is NULL");
        return;
    }

    if (!packet) {
        debugLogger("parseIcmpMessage: packet pointer is NULL");
        return;
    }

    if (!sender_addr || !sender_addr_len) {
        debugLogger("parseIcmpMessage: Fatal error: sender_addr and sender_addr_len shouldn't be NULL");
        return;
    }

    // checking that the socket address is of IPv4 family
    if (*sender_addr_len != sizeof(struct sockaddr_in)) {
        infoLogger(state->program_name, "parseIcmpMessage: sender's socket address doesn't match the length of struct sockaddr_in (this ping is only handing IPv4)");
        return;
    }

    struct ip *ip_header = NULL; // ip header
    struct icmphdr *icmp_header = NULL; // icmp header
    void *data = NULL; // icmp data (playload)
    size_t ip_header_len = 0;
    uint8_t ttl = 0;

    // if sock_type is SOCK_RAW then the ip header is sent in the packet (should be skipped)
    if (state->socket_type == SOCK_RAW) {
        ip_header = (struct ip *)packet;
        ip_header_len = ip_header->ip_hl << 2; // converting from words into bytes (x4)
        if (ip_header->ip_v != 4) {
            infoLogger(state->program_name, "parseIcmpMessage: packet send with unsupported ip version (to be ignored)");
            return; // ignore
        }
        ttl = ip_header->ip_ttl; 
    }

    if (packet_len < ip_header_len - sizeof(icmp_echo_header_t)) {
        debugLogger("parseIcmpMessage: packet_len parameter is less than ip-header and icmp-header (can't be the case)!");
        return;
    }

    size_t data_len = packet_len - ip_header_len - sizeof(icmp_echo_header_t);

    // skipping ip header to get ICMP message
    icmp_header = (struct icmphdr *)((uint8_t *)packet + ip_header_len);
    data = (uint8_t *)packet + ip_header_len + sizeof(icmp_echo_header_t);


    struct sockaddr_in *saddr = (struct sockaddr_in *)sender_addr;

    if (icmp_header->type == ICMP_ECHOREPLY) {
        // check if the sender's address is the same as the ping destination's address

        if (state->dest_addr.sin_addr.s_addr != saddr->sin_addr.s_addr) {
            debugLogger("parseIcmpMessage: received an ICMP echo reply packet from a different host than our destination (to be ignored)");
            return; // ignore packet
        }

        handle_echo_reply(state, icmp_header, data, data_len, packet_len, ttl);
        return;
    }

    if (state->verbose == 1 && (icmp_header->type == ICMP_DEST_UNREACH || icmp_header->type == ICMP_TIME_EXCEEDED || icmp_header->type == ICMP_REDIRECT)) {
        handle_error_message(saddr, icmp_header);
        return;
    }

    // any other message type is to be ignored
    return; 
}
