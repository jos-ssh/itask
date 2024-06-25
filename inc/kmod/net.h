#ifndef __INC_KMOD_NET_H
#define __INC_KMOD_NET_H

#include "inc/env.h"
#include "kmod/net/ring_buf.h"
#include <inc/kmod/request.h>
#include <stdint.h>

#ifndef NETD_VERSION
#define NETD_VERSION 0
#endif // !NETD_VERSION

#define NETD_MODNAME "jos.core.net"

enum VirtioNetRequestType {
    NETD_IDENTITY = KMOD_REQ_IDENTIFY,
    NETD_REQ_RECIEVE = KMOD_REQ_FIRST_USABLE,
    NETD_REQ_SEND,
    NETD_NREQUESTS
};

union NetdRequest {
    struct NetdRecieve {
        envid_t target;
    } recieve;
    struct NetdSend {
        size_t size;
        char data[BUFSIZE];
    } send;
    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

union NetdResponce {
    struct NetdRecieveData {
        size_t size;
        char data[BUFSIZE];
    } recieve_data;
    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

#endif /* net.h */
