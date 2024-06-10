#ifndef __INC_KMOD_NET_H
#define __INC_KMOD_NET_H

#include <inc/kmod/request.h>
#include <stdint.h>

#ifndef NETD_VERSION
#define NETD_VERSION 0
#endif // !NETD_VERSION

#define NETD_MODNAME "jos.core.net"

enum VirtioNetRequestType {
  NETD_IDENTITY = KMOD_REQ_IDENTIFY,
  NETD_IS_TEAPOT = KMOD_REQ_FIRST_USABLE,
  NETD_NREQUESTS
};

union NetdRequest {
  char req;
} __attribute__((aligned(PAGE_SIZE)));

union NetdResponce {
  char res;
} __attribute__((aligned(PAGE_SIZE)));

#endif /* net.h */
