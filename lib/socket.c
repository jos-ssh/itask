#include "inc/netinet/in.h"

#include <inc/socket.h>
#include <inc/kmod/net.h>
#include <inc/rpc.h>
#include <inc/lib.h>

static struct sockaddr_in sSocketAddr = {PF_INET, 2222, {0}};

int
getsockname(int sockfd, struct sockaddr *restrict addr,
            socklen_t *restrict addrlen) {
    memcpy(addr, &sSocketAddr, *addrlen);
    return 0;
}

int
getpeername(int sockfd, struct sockaddr *restrict addr,
            socklen_t *restrict addrlen) {
    memcpy(addr, &sSocketAddr, *addrlen);
    return 0;
}

int socket(int domain, int type, int protocol) NOTIMPLEMENTED(int);


static union NetdResponce sResponse;
static union NetdResponce *const sResponseAddr = &sResponse;

int
devsocket_recv(void *out_buf, size_t n) {
    union NetdRequest request;
    request.recieve.target = sys_getenvid();
    request.recieve.size = n;

    union NetdResponce *response = sResponseAddr;
    int res = rpc_execute(kmod_find_any_version(NETD_MODNAME), NETD_REQ_RECIEVE, &request, (void **)&response);
    if (res < 0) {
        return res;
    }
    memcpy(out_buf, response->recieve_data.data, response->recieve_data.size);
    return response->recieve_data.size;
}

int
devsocket_send(char *in_buf, size_t n) {
    union NetdRequest request;
    memcpy(request.send.data, in_buf, n);
    request.send.size = n;
    int res = rpc_execute(kmod_find_any_version(NETD_MODNAME), NETD_REQ_SEND, &request, NULL);
    return res < 0 ? res : n;
}

int
devsocket_poll() {
    union NetdRequest request;
    request.poll.target = sys_getenvid();

    int res = 0;
    while (res == 0) {
        res = rpc_execute(kmod_find_any_version(NETD_MODNAME), NETD_REQ_POLL, &request, NULL);
    }
    return 0;
}