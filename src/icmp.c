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

extern ping_state_t state;

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


int createIcmpEchoRequestMessage() {
    // (*) header
    state.packet.header.type = ICMP_ECHO;
    state.packet.header.code = 0;
    state.packet.header.identifier = htons(state.identifier); // overwritten by the kernel in case of SOCK_DGRAM socket (user lacking privileges)
    state.packet.header.sequence = htons(state.sequence);
    state.packet.header.checksum = 0;  // Will be calculated

    // (*) data
    if (state.packet.data && state.packet.data_len) {
        size_t off = 0;
        size_t len = state.packet.data_len;
        if (state.packet.data_len >= sizeof(struct timeval)) {
            // timestamp
            len -= sizeof(struct timeval);
            off += sizeof(struct timeval);
            struct timeval tv;
            gettimeofday(&tv, NULL);
            tv.tv_sec = tv.tv_sec; // not interpreted by the receiver (so no need to convert it into network byte order)
            tv.tv_usec = tv.tv_usec; // same as above
            memcpy(state.packet.data, &tv, sizeof(tv));
        }

        // filling remaining data with zeros
        for (size_t i = 0; i < len; i++) {
            state.packet.data[i + off] = 0;
        }
    }

    // (*) checksum

    // now we're ready to calculate the checksum
    state.packet.header.checksum = htons(calculateChecksum(&state.packet));

    return (ICMP_OK);
}


// (*) parsing incoming packets

// @returns ICMP_ERROR in case of error, ICMP_OK in case everything went successfully, NETWORK_NOISE in case of the network packet isn't meant for our Process
static int handle_echo_reply(struct icmphdr *icmp_header, void *data, size_t data_len, size_t icmp_message_len, uint8_t ttl) {
    if (!icmp_header) {
        debugLogger("handle_echo_reply: pointer args cannot be null");
        return (ICMP_ERROR);
    }

    // validating the identifier only if the socket type is SOCK_RAW (useless_identifier is set to 0)
    if (state.useless_identifier == 0 && state.identifier != ntohs(icmp_header->un.echo.id)) {
        infoLogger("handle_echo_reply: received packet has a different ID (to be ignored)");
        return (PARSE_NETWORK_NOISE); // ignore
    }

    uint16_t packet_sequence = ntohs(icmp_header->un.echo.sequence);
    int isDuplicate = 0;

    if (state.received[packet_sequence] == 1) {
        isDuplicate = 1;
        state.num_rept += 1; // increment number of duplicates
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

        if (useconds_diff < 0) {
            seconds_diff -= 1;
            useconds_diff += 1000000;
        }
        rrt = (seconds_diff * 1000.0) + (useconds_diff / 1000.0);

        if (!isDuplicate) {
            // update statistics trackers (store in seconds to protect from overflow)
            double rrt_s = seconds_diff + useconds_diff / 1000000.0;

            state.rrt_sum += rrt_s;
            state.rrt_sum_sq += rrt_s * rrt_s;
            if (state.num_recv == 0) {
                state.rrt_max = rrt_s;
                state.rrt_min = rrt_s;
            } else {
                if (state.rrt_max < rrt_s) {
                    state.rrt_max = rrt_s;
                }
                if (rrt_s < state.rrt_min) {
                    state.rrt_min = rrt_s;
                }
            }
        }
    }

    // (*) printing result
    if (state.quiet == 0 && state.flood == 0) {
        printf("%zu bytes from %s: icmp_seq=%u", icmp_message_len, state.display_address, packet_sequence);
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
    }

    // backspace when the packet is received back
    if (state.flood == 1 && state.quiet == 0) {
        printf("\b");
        fflush(stdout);
    }

    // packet's sequence is consumed
    state.received[packet_sequence] = 1; // mark sequence as received

    if (!isDuplicate) {
        state.num_recv += 1; // increment number of received packets
    }

    return (PARSE_OK);
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
        [ICMP_EXC_TTL] = "Time to live exceeded",
        [ICMP_EXC_FRAGTIME] = "Fragment Reass time exceeded",
    };

    if (icmp_code < sizeof(time_exceeded_messages) / sizeof(const char *)) {
        return (time_exceeded_messages[icmp_code]);
    }

    return (NULL);
}

// @returns ICMP_ERROR in case of error, ICMP_OK in case everything went successfully, NETWORK_NOISE in case of the network packet isn't meant for our Process
static int handle_error_message(struct sockaddr_in *saddr, void *data, size_t data_len, size_t icmp_message_len, struct icmphdr *icmp_header) {
    if (!saddr || !icmp_header) {
        debugLogger("handle_error_message: pointer args cannot be NULL");
        return (ICMP_ERROR);
    }

    // check if payload size matches the minimum size of an ip header and the size of icmp header (64 bits of original data)
    if (!data || data_len < sizeof(struct ip)) {
        infoLogger("handle_error_message: payload too small to include original ip header!"); 
        return (PARSE_NETWORK_NOISE);
    }

    // first 20 byte of the original ip header
    struct ip orig_ip;
    memcpy(&orig_ip, data, sizeof(struct ip));

    // ip header length should be between 5 and 15 (5 * 4 = 20; 15 * 4 = 60)
    if (!(orig_ip.ip_hl >= 5 && orig_ip.ip_hl <= 15)) {
        infoLogger("handle_error_message: invalid original IP header length");
        return (PARSE_NETWORK_NOISE);
    }

    size_t orig_ip_header_len = orig_ip.ip_hl << 2;

    if (data_len < orig_ip_header_len + sizeof(struct icmphdr)) {
        infoLogger("handle_error_message: payload too small to include original ip header and original icmp header (first 64 bits of original data)!");
        return (PARSE_NETWORK_NOISE);
    }

    // icmp header of the original packet (first 64 bits of original data)
    struct icmphdr orig_icmp;
    memcpy(&orig_icmp, (uint8_t*)data + orig_ip_header_len, sizeof(struct icmphdr));

    // check if the destination of the original message (the error is about) has the same destination as our ping destination
    if (orig_ip.ip_dst.s_addr != state.dest_addr.sin_addr.s_addr) {
        infoLogger("handle_error_message: the original ip header has a different destination than ping's destination");
        return (PARSE_NETWORK_NOISE); // ignore
    }

    // check if it has the same protocol (ICMP)
    if (orig_ip.ip_p != IPPROTO_ICMP) {
        infoLogger("handle_error_message: the original ip header has a different protocol than ICMP");
        return (PARSE_NETWORK_NOISE); // ignore
    }

    // check if the identifier is the same
    if (state.useless_identifier == 0 && ntohs(orig_icmp.un.echo.id) != state.identifier) {
        infoLogger("handle_error_message: the original ip header has a different ICMP id");
        return (PARSE_NETWORK_NOISE); // ignore (this error is for another process)
    }

    if (state.quiet == 0 && state.flood == 0) {
        printf("%zu bytes from %s: ", icmp_message_len, inet_ntoa(saddr->sin_addr));
    }

    if (icmp_header->type == ICMP_DEST_UNREACH) {
        const char *message = get_unreach_message(icmp_header->code);

        if (state.quiet == 1) {
            return (PARSE_OK);
        } else if (state.flood == 1) {
            printf("\b");
            fflush(stdout);
            return (PARSE_OK);
        } else if (message) {
            printf("%s\n", message);
        } else {
            printf("Destination Unreachable (unknown code: %d)\n", icmp_header->code);
        }
        return (PARSE_OK);
    }

    // time exceeded message
    if (icmp_header->type == ICMP_TIME_EXCEEDED) {
        const char *message = get_time_exceeded_message(icmp_header->code);

        if (state.quiet == 1) {
            return (PARSE_OK);
        } else if (state.flood == 1) {
            printf("\b");
            fflush(stdout);
            return (PARSE_OK);
        } else if (message) {
            printf("%s\n", message);
        } else {
            printf("Time Exceeded (unknown code: %d)\n", icmp_header->code);
        }
        return (PARSE_OK);
    }

    // redirect message
    if (icmp_header->type == ICMP_REDIRECT) {
        const char *message = get_redirect_message(icmp_header->code);
        if (state.quiet == 1) {
            return (PARSE_OK);
        } else if (state.flood == 1) {
            printf("\b");
            fflush(stdout);
            return (PARSE_OK);
        } else if (message) {
            printf("%s\n", message); 
        } else {
            printf("Redirect (Unknown code: %d)\n", icmp_header->code);
        }
        return (PARSE_OK);
    }
    return (PARSE_NETWORK_NOISE); // ignore any other type
}

int parseIcmpMessageAndLogResult(void *packet, size_t packet_len, struct sockaddr *sender_addr, socklen_t *sender_addr_len) {
    if (!packet || !sender_addr || !sender_addr_len) {
        debugLogger("parseIcmpMessage: pointer args cannot be NULL");
        return (ICMP_ERROR);
    }

    if (*sender_addr_len != sizeof(struct sockaddr_in)) {
        debugLogger("parseIcmpMessage: sender's socket address doesn't match the length of struct sockaddr_in");
        return (ICMP_ERROR);
    }

    struct ip *ip_header = NULL; // ip header
    size_t ip_header_len = 0;
    uint8_t ttl = 0;

    // if sock_type is SOCK_RAW then the ip header is sent in the packet (should be skipped)
    if (state.socket_type == SOCK_RAW) {
        if (packet_len < sizeof(struct ip)) {
            debugLogger("parseIcmpMessage: packet_len smaller than minimum IP header size");
            return (ICMP_ERROR);
        }
        ip_header = (struct ip *)packet;
        ip_header_len = ip_header->ip_hl << 2; // converting from words into bytes (x4)
        if (ip_header->ip_v != 4) {
            infoLogger("parseIcmpMessage: packet sent with unsupported ip version (to be ignored)");
            return (PARSE_NETWORK_NOISE); // ignore
        }
        // ip header length should be between 5 and 15 (5 * 4 = 20; 15 * 4 = 60)
        if (!(ip_header->ip_hl >= 5 && ip_header->ip_hl <= 15)) {
            infoLogger("parseIcmpMessage: invalid IP header length");
            return (PARSE_NETWORK_NOISE);
        }
        ttl = ip_header->ip_ttl; 
    }

    if (packet_len < ip_header_len + sizeof(struct icmphdr)) {
        debugLogger("parseIcmpMessage: packet_len parameter is less than ipheaderlen + icmpheaderlen (can't be the case)!");
        return (ICMP_ERROR);
    }

    struct icmphdr icmp_header;
    struct sockaddr_in *saddr = (struct sockaddr_in *)sender_addr;
    size_t data_len = packet_len - ip_header_len - sizeof(struct icmphdr);
    void *data = (uint8_t *)packet + ip_header_len + sizeof(struct icmphdr);

    memcpy(&icmp_header, (uint8_t*)packet + ip_header_len, sizeof(icmp_header));

    if (icmp_header.type == ICMP_ECHOREPLY) {
        // check if the sender's address is the same as the ping destination's address
        if (state.dest_addr.sin_addr.s_addr != saddr->sin_addr.s_addr) {
            debugLogger("parseIcmpMessage: received an ICMP echo reply packet from a different host than our destination (to be ignored)");
            return (PARSE_NETWORK_NOISE); // ignore packet
        }

        return (handle_echo_reply(&icmp_header, data, data_len, packet_len - ip_header_len, ttl));
    }

    // ICMP error messages are handled and reported by default (unless suppressed by quiet mode)
    if (icmp_header.type == ICMP_DEST_UNREACH || icmp_header.type == ICMP_TIME_EXCEEDED || icmp_header.type == ICMP_REDIRECT) {
        return (handle_error_message(saddr, data, data_len, packet_len - ip_header_len, &icmp_header));
    }

    // any other message type is to be ignored
    return (PARSE_NETWORK_NOISE);
}
