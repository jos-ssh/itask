#pragma once

#include "inc/types.h"

#define BUFSIZE 1024

struct RingBuffer {
    _Atomic volatile size_t head;
    _Atomic volatile size_t tail;
    char data[BUFSIZE];
};


int read_buf(struct RingBuffer* buf, char* out_buf, size_t n);
void write_buf(struct RingBuffer* buf, const char* in_buf, size_t n);
