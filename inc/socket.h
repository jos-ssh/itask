#pragma once

#include <inc/types.h>

/* Type for length arguments in socket calls.  */
typedef unsigned int socklen_t;
typedef unsigned short int sa_family_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

struct sockaddr_storage {
    sa_family_t ss_family; /* Address family */
};

int getsockname(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen);

int getpeername(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen);

int socket(int domain, int type, int protocol);

int opensock(void);

// TEMPORARY HERE
int devsocket_send(char *in_buf, size_t n);

int devsocket_recv(void *out_buf, size_t n);

int devsocket_poll();
