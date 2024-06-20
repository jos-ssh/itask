#include "queue.h"
#include "inc/assert.h"
#include "inc/pool_alloc.h"
#include "inc/stdio.h"
#include "net.h"
#include <inc/lib.h>
#include <stdbool.h>
#include <string.h>

static void* sendq_index_to_ptr[VIRTQ_SIZE];

void
setup_queue(struct virtq *queue, volatile struct virtio_pci_common_cfg_t *cfg_header) {
    cfg_header->queue_select = queue->queue_idx;

    // force allocation
    force_alloc(queue, sizeof(struct virtq));

    cfg_header->queue_desc   = get_phys_addr(&queue->desc);
    cfg_header->queue_avail  = get_phys_addr(&queue->avail);
    cfg_header->queue_used   = get_phys_addr(&queue->used);
    cfg_header->queue_enable = 1;
    cfg_header->queue_size   = VIRTQ_SIZE;
    queue->log2_size = 6;

    queue->desc_free_count = 1 << queue->log2_size;
    for (int i = queue->desc_free_count; i > 0; --i) {
        queue->desc[i - 1].next = queue->desc_first_free;
        queue->desc_first_free = i - 1;
    }
}

void
notify_queue(struct virtq *queue) {
    *((uint64_t *)queue->notify_reg) = queue->queue_idx;
}

void
queue_avail(struct virtq *queue, uint32_t count) {
    uint32_t mask = ~-(1 << queue->log2_size);

    bool skip = false;
    uint32_t avail_head = queue->avail.idx;
    uint32_t chain_start = ~((uint32_t)0);
    for (uint32_t i = 0; i < count; ++i) {
        if (!skip) {
            chain_start = i;
        }

        skip = queue->desc[i].flags & VIRTQ_DESC_F_NEXT;

        // Write an entry to the avail ring telling virtio to
        // look for a chain starting at chain_start, if this is the end
        // of a chain (end because next is false)
        if (!skip && chain_start != ~((uint32_t)0))
            queue->avail.ring[avail_head++ & mask] = chain_start;
    }
    // cprintf("avail head %d\n", avail_head);
    queue->avail.used_event = avail_head - 1;

    // Update idx (tell virtio where we would put the next new item)
    // enforce ordering until after prior store is globally visible
    queue->avail.idx = avail_head;
}

struct virtq_desc *
alloc_desc(struct virtq *queue, int writable) {
    if (queue->desc_free_count == 0)
        return NULL;

    --queue->desc_free_count;

    struct virtq_desc *desc = &queue->desc[queue->desc_first_free];
    queue->desc_first_free = desc->next;

    desc->flags = 0;
    desc->next = -1;

    if (writable)
        desc->flags |= VIRTQ_DESC_F_WRITE;

    return desc;
}

struct virtio_packet_t* allocate_virtio_packet() {
    struct send_buffer_t *buff = pool_allocator_alloc_object(&net.alloc);
    return &buff->packet;
}

void send_virtio_packet(struct virtio_packet_t* packet) {
    struct virtq_desc* desc = alloc_desc(&net.sendq, 0);
    desc->len = sizeof(*packet);
    desc->addr = get_phys_addr(packet);
    packet->vheader.gso_type = VIRTIO_NET_HDR_GSO_NONE;

    size_t desc_indx = desc - net.sendq.desc;
    sendq_index_to_ptr[desc_indx] = packet;

    queue_avail(&net.sendq, 1);
    notify_queue(&net.sendq);
}

// TODO: INVESTIGATE
void
process_queue(struct virtq *queue, bool incoming) {
    // Костыль, ибо чета код этого GC иногда много раз прозодит по одному элементу
    bool processed[VIRTQ_SIZE];
    memset(processed, 0, sizeof(processed));

    size_t tail = queue->used_tail;
    size_t const mask = ~-(1 << queue->log2_size);
    size_t const done_idx = queue->used.idx;

    do {
        struct virtq_used_elem *used = &queue->used.ring[tail & mask];
        uint16_t id = used->id;

        if (!processed[id]) {
            processed[id] = true;

            if (incoming) {
                process_packet(queue, id);
            } else if (sendq_index_to_ptr[id] != NULL) { // максимальная хуета, по идее должно отсеиваться processed, но нет
                char* packet_ptr = (char*) sendq_index_to_ptr[id];
                pool_allocator_free_object(&net.alloc, packet_ptr - offsetof(struct send_buffer_t, packet));
                sendq_index_to_ptr[id] = NULL;
            }
        }

        unsigned freed_count = 1;

        uint16_t end = id;
        while (queue->desc[end].flags & VIRTQ_DESC_F_NEXT) {
            end = queue->desc[end].next;
            ++freed_count;

            // This loop is unexpected, because all packages fit into one buffer
            panic("Mem leak in this case, consider move mem managment code from above here");
        }

        queue->desc[end].next = queue->desc_first_free;
        queue->desc_first_free = id;
        queue->desc_free_count += freed_count;
    } while ((++tail & 0xFFFF) != done_idx);

    queue->used_tail = tail;
}