#include <inc/socket.h>
#include <inc/lib.h>

int getsockname(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen) NOTIMPLEMENTED(int)

int getpeername(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen) NOTIMPLEMENTED(int)

int socket(int domain, int type, int protocol) NOTIMPLEMENTED(int)