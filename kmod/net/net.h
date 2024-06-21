#pragma once

#include "queue.h"
#include <inc/env.h>
#include <stdint.h>
#include <inc/pool_alloc.h>

extern envid_t g_InitdEnvid;
extern envid_t g_PcidEnvid;
extern bool g_SessionComplete;

extern struct virtio_net_device_t* net;

#define RECEIVE_ADDR 0x0FFFF000

#define VENDOR_ID 0x1AF4
#define DEVICE_ID 0x1000

#define SEND_BUF_NUM 512

#define UNWRAP(res, line) do { if (res != 0) { panic(line ": %i", res); } } while (0)

#ifndef trace_net
#define trace_net 0
#endif

struct recv_buffer {
    _Alignas(PAGE_SIZE) char _[PAGE_SIZE];
};

struct send_buffer_t {
    struct List _;
    struct virtio_packet_t packet;
};

void initialize();

void process_packet(struct virtio_net_hdr* net_hdr);
void *reverse_recv_buffer_addr(int64_t index);

void serve_teapot();