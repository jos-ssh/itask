#pragma once

#include <inc/socket.h>

#define PF_INET  2  /* IP protocol family.  */
#define PF_INET6 10 /* IP version 6.  */

/* Internet address.  */
typedef uint32_t in_addr_t;
/* Type to represent a port.  */
typedef uint16_t in_port_t;

struct in_addr {
    in_addr_t s_addr;
};

struct in6_addr {
    unsigned char s6_addr[16]; /* IPv6 address */
};

struct sockaddr_in {
    sa_family_t sin_family;  /* AF_INET */
    in_port_t sin_port;      /* Port number */
    struct in_addr sin_addr; /* IPv4 address */
};


struct sockaddr_in6 {
    sa_family_t sin6_family;   /* AF_INET6 */
    in_port_t sin6_port;       /* port number */
    uint32_t sin6_flowinfo;    /* IPv6 flow information */
    struct in6_addr sin6_addr; /* IPv6 address */
    uint32_t sin6_scope_id;    /* Scope ID (new in Linux 2.4) */
};
