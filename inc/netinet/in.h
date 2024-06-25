#pragma once

#include <inc/socket.h>

#define PF_INET 0xBAD

struct in6_addr {
    unsigned char s6_addr[16]; /* IPv6 address */
};

struct sockaddr_in6 {
    sa_family_t sin6_family;   /* AF_INET6 */
    in_port_t sin6_port;       /* port number */
    uint32_t sin6_flowinfo;    /* IPv6 flow information */
    struct in6_addr sin6_addr; /* IPv6 address */
    uint32_t sin6_scope_id;    /* Scope ID (new in Linux 2.4) */
};
