#include <inc/socket.h>
#include "inc/kmod/net.h"

#include <inc/lib.h>

int getsockname(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen) NOTIMPLEMENTED(int)

int getpeername(int sockfd, struct sockaddr *restrict addr,
                socklen_t *restrict addrlen) NOTIMPLEMENTED(int)

int socket(int domain, int type, int protocol) NOTIMPLEMENTED(int)


static union NetdResponce sResponse;
static union NetdResponce * const sResponseAddr = &sResponse;

int
socket_recieve(char *out_buf, size_t n) {
    union NetdRequest request;
    request.recieve.target = CURENVID;

    union NetdResponce *response = sResponseAddr; 
    int res = rpc_execute(kmod_find_any_version(NETD_MODNAME), NETD_REQ_RECIEVE, &request, (void **)&response);
    printf("recieved %ld\n", response->recieve_data.size);
    return res;
}

// int
// socket_send(char *in_buf, size_t n) {
//     union NetdRequest
// }
