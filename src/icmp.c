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
static uint16_t calculateChecksum(icmp_echo_t *request) {
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
            tv.tv_sec = tv.tv_sec; // not interpreted by the receiver (so no need to convert it into network byte order)
            tv.tv_usec = tv.tv_usec; // same as above
            memcpy(ping_state->packet.data, &tv, sizeof(tv));
        }

        // filling remaining data with zeros
        for (size_t i = 0; i < len; i++) {
            ping_state->packet.data[i + off] = 0;
        }
    }

    // (*) checksum

    // now we're ready to calculate the checksum
    ping_state->packet.header.checksum = htons(calculateChecksum(&ping_state->packet));

    return (ICMP_OK);
}


// (*) parsing incoming packets

static void handle_echo_reply(ping_state_t *state, struct icmphdr *icmp_header, void *data, size_t data_len, size_t packet_len, uint8_t ttl) {
    if (!icmp_header || !state) {
        debugLogger("handle_echo_reply: pointer args cannot be null");
        return;
    }

    // validating the identifier only if the socket type is SOCK_RAW (useless_identification is set to 0)
    if (state->useless_identifier == 0 && state->identifier != ntohs(icmp_header->un.echo.id)) {
        infoLogger(state->program_name, "handle_echo_reply: received packet has a different ID (to be ignored)");
        return; // ignore
    }

    uint16_t packet_sequence = ntohs(icmp_header->un.echo.sequence);
    int isDuplicate = 0;

    if (state->received[packet_sequence] == 1) {
        isDuplicate = 1;
        state->num_rept += 1; // increment number of duplicates
    }

    // RRT (round-trip time)
    float rrt = -1;

    if (data_len >= sizeof(struct timeval)) {
        struct timeval sent_time;
        struct timeval reply_time;

        // using memcpy to prevent alignment issues in some systems (good practice)
        memcpy(&sent_time, data, sizeof(sent_time));

        gettimeofday(&reply_time, NULL);
        long seconds_diff = reply_time.tv_sec - sent_time.tv_sec; // sent_time fields are not sent in network byte order (so they are fine)
        long useconds_diff = reply_time.tv_usec - sent_time.tv_usec;

        rrt = (seconds_diff * 1000.0) + (useconds_diff / 1000.0);
    }

    // (*) printing result
    printf("%lu bytes from %s: icmp_seq=%u", packet_len, state->display_address, packet_sequence);
    // include ttl only in case of SOCK_RAW
    if (ttl != 0) {
        printf(" ttl=%u", ttl);
    }
    if (rrt >= 0) {
        printf(" time=%.3f ms", rrt);
    }
    if (isDuplicate) {
        printf (" (DUP!)");
    }
    printf("\n");

    // packet's sequence is consumed
    state->received[packet_sequence] = 1; // mark sequence as received

    if (!isDuplicate) {
        state->num_recv += 1; // increment number of received packets
    }
}

// helper
// @brief given the icmp code of the ICMP dest unreachable error message, it returns the correspondent result or NULL if no such code is supported
static const char *get_unreach_message(uint8_t icmp_code) {
    static const char *unreach_messages[] = {
        [ICMP_NET_UNREACH] = "Destination Net Unreachable",
        [ICMP_HOST_UNREACH] = "Destination Host Unreachable",
        [ICMP_PROT_UNREACH] = "Destination Protocol Unreachable",
        [ICMP_PORT_UNREACH] = "Destination Port Unreachable",
        [ICMP_FRAG_NEEDED] = "Fragmentation needed and DF set",
        [ICMP_SR_FAILED] = "Source route failed",
        [ICMP_NET_UNKNOWN] = "Destination Net Unknown",
        [ICMP_HOST_UNKNOWN] = "Destination Host Unknown",
        [ICMP_HOST_ISOLATED] = "Destination Host Isolated",
        [ICMP_NET_ANO] = "Network administratively prohibited",
        [ICMP_HOST_ANO] = "Host administratively prohibited",
        [ICMP_NET_UNR_TOS] = "Network unreachable for TOS",
        [ICMP_HOST_UNR_TOS] = "Host unreachable for TOS",
        [ICMP_PKT_FILTERED] = "Packet filtered",
        [ICMP_PREC_VIOLATION] = "Precedence violation",
        [ICMP_PREC_CUTOFF] = "Precedence cut off",
    };

    if (icmp_code < sizeof(unreach_messages) / sizeof(const char *)) {
        return (unreach_messages[icmp_code]);
    }

    return (NULL);
}

// helper
// @brief given the icmp code of the ICMP redirect error message, it returns the correspondent result or NULL if no such code is supported
static const char *get_redirect_message(uint8_t icmp_code) {
    static const char *redirect_messages[] = {
        [ICMP_REDIR_NET] = "Redirect Net",
        [ICMP_REDIR_HOST] = "Redirect Host",
        [ICMP_REDIR_NETTOS] = "Redirect Net for TOS",
        [ICMP_REDIR_HOSTTOS] = "Redirect Host for TOS",
    };

    if (icmp_code < sizeof(redirect_messages) / sizeof(const char *)) {
        return (redirect_messages[icmp_code]);
    }

    return (NULL);
}

// helper
// @brief given the icmp code of the ICMP dest time exceeded error message, it returns the correspondent result or NULL if no such code is supported
static const char *get_time_exceeded_message(uint8_t icmp_code) {
    static const char *time_exceeded_messages[] = {
        [ICMP_EXC_TTL] = "TTL count exceeded",
        [ICMP_EXC_FRAGTIME] = "Fragment Reass time exceeded",
    };

    if (icmp_code < sizeof(time_exceeded_messages) / sizeof(const char *)) {
        return (time_exceeded_messages[icmp_code]);
    }

    return (NULL);
}

static void handle_error_message(ping_state_t *state, struct sockaddr_in *saddr, void *data, size_t data_len, struct icmphdr *icmp_header) {
    if (!state || !saddr || !icmp_header) {
        debugLogger("handle_error_message: pointer args cannot be NULL");
        return;
    }

    // check if payload size matches the minimum size of an ip header and the size of icmp header (64 bits of original data)
    if (!data || data_len < sizeof(struct ip)) {
        infoLogger(state->program_name, "handle_error_message: playload too small to include original ip header!"); 
        return;
    }

    // first 20 byte of the original ip header
    struct ip orig_ip;
    memcpy(&orig_ip, data, sizeof(struct ip));

    // ip header length should be between 5 and 15 (5 * 4 = 20; 15 * 4 = 60)
    if (!(orig_ip.ip_hl >= 5 && orig_ip.ip_hl <= 15)) {
        infoLogger(state->program_name, "handle_error_message: invalid original IP header length");
        return;
    }

    size_t orig_ip_header_len = orig_ip.ip_hl << 2;

    if (data_len < orig_ip_header_len + sizeof(struct icmphdr)) {
        infoLogger(state->program_name, "handle_error_message: payload too small to include original ip header and original icmp header (first 64 bits of original data)!");
        return;
    }

    // icmp header of the original packet (first 64 bits of original data)
    struct icmphdr orig_icmp;
    memcpy(&orig_icmp, (uint8_t*)data + orig_ip_header_len, sizeof(struct icmphdr));

    // check if the distination of the original message (the error is about) has the same destination as our ping destination
    if (orig_ip.ip_dst.s_addr != state->dest_addr.sin_addr.s_addr) {
        infoLogger(state->program_name, "handle_error_message: the original ip header has a different distination than ping's destination");
        return; // ignore
    }

    // check if it has the same protocol (ICMP)
    if (orig_ip.ip_p != IPPROTO_ICMP) {
        infoLogger(state->program_name, "handle_error_message: the original ip header has a different protocol than ICMP");
        return; // ignore
    }

    // check if the identifier is the same
    if (state->useless_identifier == 0 && ntohs(orig_icmp.un.echo.id) != state->identifier) {
        infoLogger(state->program_name, "handle_error_message: the original ip header has a different ICMP id");
        return; // ignore (this error is for another process)
    }

    printf("From %s icmp_seq=%u", inet_ntoa(saddr->sin_addr), ntohs(orig_icmp.un.echo.sequence));

    if (icmp_header->type == ICMP_DEST_UNREACH) {
        const char *message = get_unreach_message(icmp_header->code);

        if (message) {
            printf(" %s\n", message);
        } else {
            printf(" Destination Unreachable (unknown code: %d)\n", icmp_header->code);
        }
        return;
    }

    // time exceeded message
    if (icmp_header->type == ICMP_TIME_EXCEEDED) {
        const char *message = get_time_exceeded_message(icmp_header->code);
        if (message) {
            printf(" %s\n", message); 
        } else {
            printf(" Time Exceeded (unknown code: %d)\n", icmp_header->code);
        }
        return;
    }

    // redirect message
    if (icmp_header->type == ICMP_REDIRECT) {
        const char *message = get_redirect_message(icmp_header->code);
        if (message) {
            printf(" %s\n", message); 
            return;
        } else {
            printf(" Redirect (Unknown code: %d)\n", icmp_header->code);
        }
        return;
    }
}

void parseIcmpMessage(ping_state_t *state, void *packet, size_t packet_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len) {
    if (!state || !packet || !sender_addr || !sender_addr_len) {
        debugLogger("parseIcmpMessage: pointer args cannot be NULL");
        return;
    }

    if (*sender_addr_len != sizeof(struct sockaddr_in)) {
        infoLogger(state->program_name, "parseIcmpMessage: sender's socket address doesn't match the length of struct sockaddr_in (this ping is only handing IPv4)");
        return;
    }

    struct ip *ip_header = NULL; // ip header
    size_t ip_header_len = 0;
    uint8_t ttl = 0;

    // if sock_type is SOCK_RAW then the ip header is sent in the packet (should be skipped)
    if (state->socket_type == SOCK_RAW) {
        ip_header = (struct ip *)packet;
        ip_header_len = ip_header->ip_hl << 2; // converting from words into bytes (x4)
        if (ip_header->ip_v != 4) {
            infoLogger(state->program_name, "parseIcmpMessage: packet sent with unsupported ip version (to be ignored)");
            return; // ignore
        }
        // ip header length should be between 5 and 15 (5 * 4 = 20; 15 * 4 = 60)
        if (!(ip_header->ip_hl >= 5 && ip_header->ip_hl <= 15)) {
            infoLogger(state->program_name, "parseIcmpMessage: invalid IP header length");
            return;
        }
        ttl = ip_header->ip_ttl; 
    }

    if (packet_len < ip_header_len + sizeof(struct icmphdr)) {
        debugLogger("parseIcmpMessage: packet_len parameter is less than ipheaderlen + icmpheaderlen (can't be the case)!");
        return;
    }

    struct icmphdr icmp_header;
    struct sockaddr_in *saddr = (struct sockaddr_in *)sender_addr;
    size_t data_len = packet_len - ip_header_len - sizeof(struct icmphdr);
    void *data = (uint8_t *)packet + ip_header_len + sizeof(struct icmphdr);

    memcpy(&icmp_header, (uint8_t*)packet + ip_header_len, sizeof(icmp_header));

    if (icmp_header.type == ICMP_ECHOREPLY) {
        // check if the sender's address is the same as the ping destination's address
        if (state->dest_addr.sin_addr.s_addr != saddr->sin_addr.s_addr) {
            debugLogger("parseIcmpMessage: received an ICMP echo reply packet from a different host than our destination (to be ignored)");
            return; // ignore packet
        }

        handle_echo_reply(state, &icmp_header, data, data_len, packet_len, ttl);
        return;
    }

    // in case of icmp error message (verbose option must be specified)
    if (state->verbose == 1 && (icmp_header.type == ICMP_DEST_UNREACH || icmp_header.type == ICMP_TIME_EXCEEDED || icmp_header.type == ICMP_REDIRECT)) {
        handle_error_message(state, saddr, data, data_len, &icmp_header);
        return;
    }

    // any other message type is to be ignored
    return; 
}
