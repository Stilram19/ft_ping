#ifndef ICMP_H
#define ICMP_H

#include "ft_ping.h"

#define ICMP_ERROR -1
#define ICMP_OK 0

// @brief fills the given request with an ICMP packet 
// @param ping_state takes the ping_state structure where it can find the current sequence, identifier... (useful info when constructing the ICMP header)
// * this function is not responsible of releasing any resources (memory...)
// @return returns ICMP_ERROR in case of error, ICMP_OK otherwise
int createIcmpEchoRequestMessage(ping_state_t *ping_state);

#endif
