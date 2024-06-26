#pragma once

#include "ethernet.h"
#include "inc/env.h"
#include "inc/mmu.h"
#include "ring_buf.h"


enum ConnectionState {
    kCreated,
    kConnected,
    kFinished
};


enum ProcessingState {
    NO_PACKETS,
    ARP_RECIEVED,
    CONNECTION_PROCESSED,
    SEND_PROCESSED,
    RECIEVE_PROCESSED,
    CONNECTION_ESTABLISHED,
    CONNECTION_CLOSED,
};

struct Connection {
    struct RingBuffer recieve_buf;
    _Atomic volatile int state;
    struct tcp_hdr_t client; // TODO:
} __attribute__((aligned(PAGE_SIZE)));

extern struct Connection g_Connection;
extern struct RingBuffer g_SendBuffer;

struct Message {
    size_t size;
    char data[BUFSIZE];
} __attribute__((packed));


void netd_process_loop(envid_t parent);
