#pragma once

#include <inc/types.h>

/* Type for length arguments in socket calls.  */
typedef unsigned int socklen_t;

struct sockaddr {
    unsigned short int sa_family;
    unsigned char sa_data[14];
};

/* POSIX.1g specifies this type name for the `sa_family' member.  */
typedef unsigned short int sa_family_t;

/* This macro is used to declare the initial common members
   of the data types used for socket addresses, `struct sockaddr',
   `struct sockaddr_in', `struct sockaddr_un', etc.  */

#define __SOCKADDR_COMMON(sa_prefix) \
    sa_family_t sa_prefix##family

#define __SOCKADDR_COMMON_SIZE (sizeof(unsigned short int))


/* Type to represent a port.  */
typedef uint16_t in_port_t;

/* Internet address.  */
typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
};

/* Structure describing an Internet socket address.  */
struct sockaddr_in {
    __SOCKADDR_COMMON(sin_);
    in_port_t sin_port;      /* Port number.  */
    struct in_addr sin_addr; /* Internet address.  */

    /* Pad to size of `struct sockaddr'.  */
    unsigned char sin_zero[sizeof(struct sockaddr) - __SOCKADDR_COMMON_SIZE - sizeof(in_port_t) - sizeof(struct in_addr)];
};

struct sockaddr_storage {
    unsigned short int ss_family; /* Address family */
};

int getsockname(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen);

int getpeername(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen);

#define PF_INET  2  /* IP protocol family.  */
#define PF_INET6 10 /* IP version 6.  */