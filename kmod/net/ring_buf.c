#include <inc/types.h>
#include <stdatomic.h>

#define BUFSIZE 1024

struct RingBuffer {
    _Atomic volatile size_t head;
    _Atomic volatile size_t tail;
    char data[BUFSIZE];
};


static size_t
get_idx(_Atomic volatile size_t* val) {
    return atomic_load(val) % BUFSIZE;
}

void
read_buf(struct RingBuffer* buf, char* out_buf, size_t n) {
    size_t can_grab = get_idx(&buf->tail) - get_idx(&buf->head);
    size_t can_read = MIN(can_grab, n);

    for (size_t idx = 0; idx < can_read; ++idx) {
        out_buf[idx] = buf->data[(get_idx(&buf->head) + idx) & BUFSIZE];
    }

    atomic_fetch_add(&buf->head, can_read);
}


void
write_buf(struct RingBuffer* buf, char* in_buf, size_t n) {
    size_t tail = get_idx(&buf->tail);

    for (size_t idx = 0; idx < n; ++idx) {
        buf->data[(idx + tail) % BUFSIZE] = in_buf[idx];
    }
    atomic_fetch_add(&buf->tail, n);
}
