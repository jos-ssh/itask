#pragma once

#include <inc/socket.h>

int inet_pton(int af, const char *restrict src, void *restrict dst);

#define AF_INET  PF_INET
#define AF_INET6 PF_INET6
