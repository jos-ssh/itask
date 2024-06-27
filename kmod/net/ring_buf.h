#pragma once

#include "inc/mmu.h"
#include "inc/types.h"

#define BUFSIZE 4096

struct RingBuffer {
    _Atomic volatile size_t head;
    _Atomic volatile size_t tail;
    unsigned char data[BUFSIZE];
} __attribute__((aligned(PAGE_SIZE)));


int read_buf(struct RingBuffer* buf, unsigned char* out_buf, size_t n);
void write_buf(struct RingBuffer* buf, const unsigned char* in_buf, size_t n);
