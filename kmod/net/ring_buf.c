#include "ring_buf.h"
#include "inc/stdio.h"
#include "inc/types.h"

#include <stdatomic.h>

static size_t
get_idx(_Atomic volatile size_t* val) {
    return atomic_load(val) % BUFSIZE;
}

int
read_buf(struct RingBuffer* buf, unsigned char* out_buf, size_t n) {
    size_t head = get_idx(&buf->head);
    size_t can_grab = get_idx(&buf->tail) - head;
    size_t can_read = MIN(can_grab, n);

    for (size_t idx = 0; idx < can_read; ++idx) {
        out_buf[idx] = buf->data[(head + idx) % BUFSIZE];
    }

    atomic_fetch_add(&buf->head, can_read);
    return can_read;
}


void
write_buf(struct RingBuffer* buf, const unsigned char* in_buf, size_t n) {
    size_t tail = get_idx(&buf->tail);

    for (size_t idx = 0; idx < n; ++idx) {
        buf->data[(idx + tail) % BUFSIZE] = in_buf[idx];
    }
    atomic_fetch_add(&buf->tail, n);
}

int
size_buf(struct RingBuffer* buf) {
    return get_idx(&buf->tail) - get_idx(&buf->head);
}