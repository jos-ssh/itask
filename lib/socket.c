#include "inc/error.h"
#include "inc/fcntl.h"
#include "inc/netinet/in.h"

#include <inc/socket.h>
#include <inc/kmod/net.h>
#include <inc/rpc.h>
#include <inc/lib.h>
#include <inc/poll.h>

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

static ssize_t devsock_read(struct Fd *, void *, size_t);
static ssize_t devsock_write(struct Fd *, const void *, size_t);
static int devsock_stat(struct Fd *, struct Stat *);
static int devsock_poll(struct Fd *);

static int
devsock_close(struct Fd *fd) {
    // TODO: actually close connection
    USED(fd);
    return 0;
}

struct Dev devsock = {
        .dev_id = 's',
        .dev_name = "sock",
        .dev_read = devsock_read,
        .dev_write = devsock_write,
        .dev_close = devsock_close,
        .dev_stat = devsock_stat,
        .dev_poll = devsock_poll};

int socket(int domain, int type, int protocol) NOTIMPLEMENTED(int);

int
opensock(void) {
    int res;
    struct Fd *fd;
    if ((res = fd_alloc(&fd)) < 0) return res;

    if ((res = sys_alloc_region(0, fd, PAGE_SIZE, PROT_RW | PROT_SHARE)) < 0) return res;

    fd->fd_dev_id = devsock.dev_id;
    fd->fd_omode = O_RDWR;
    return fd2num(fd);
}

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
    size_t size = response->recieve_data.size;
    memcpy(out_buf, response->recieve_data.data, response->recieve_data.size);
    sys_unmap_region(CURENVID, (void *)sResponseAddr, PAGE_SIZE);
    return size;
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
    // while (res == 0) {
    res = rpc_execute(kmod_find_any_version(NETD_MODNAME), NETD_REQ_POLL, &request, NULL);
    // }
    /*
    if (res >= 0) {
      printf("socket has %d bytes of data\n", res);
    }
    else {
      printf("poll err: %i\n", res);
    }
    */
    return res;
}

static ssize_t
devsock_read(struct Fd *fd, void *buf, size_t bufsiz) {
    if (fd->fd_sock.is_closed) { return 0; }

    int res = 0;
    const bool repeat = !!(fd->fd_omode & O_NONBLOCK);
    do {
        res = devsocket_recv(buf, bufsiz);
        if (res == -E_CONNECTION_СLOSED) {
            fd->fd_sock.is_closed = true;
        }
        if (res != 0) {
            return res;
        }
    } while (repeat);

    return -E_WOULD_BLOCK;
}

static ssize_t
devsock_write(struct Fd *fd, const void *buf, size_t bufsiz) {
    if (fd->fd_sock.is_closed) { return 0; }

    int res = devsocket_send((char *)buf, bufsiz);
    if (res == -E_CONNECTION_СLOSED) {
        fd->fd_sock.is_closed = true;
    }

    return res;
}

static int
devsock_stat(struct Fd *fd, struct Stat *stat) {
    strcpy(stat->st_name, "<sock>");

    stat->st_mode = IFSOCK;
    stat->st_size = 0;
    stat->st_isdir = 0;
    stat->st_dev = &devsock;
    return 0;
}

static int
devsock_poll(struct Fd *fd) {
    if (fd->fd_sock.is_closed) { return POLLHUP | POLLIN | POLLOUT; }

    int res = devsocket_poll();
    if (res == -E_CONNECTION_СLOSED) {
        fd->fd_sock.is_closed = true;
        return POLLHUP | POLLIN | POLLOUT;
    }

    if (res < 0) {
        return POLLERR;
    }

    if (res > 0) {
        return POLLIN | POLLOUT;
    }

    return POLLOUT;
}
