#include "queue.h"
#include "inc/stdio.h"
#include "net.h"
#include <inc/lib.h>

void
setup_queue(struct virtq *queue, volatile struct virtio_pci_common_cfg_t *cfg_header) {
    cfg_header->queue_select = queue->queue_idx;

    // force allocation
    queue->desc[0].addr = 0;
    queue->avail.flags = 0;
    queue->used.flags = 0;

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
    static bool mapped = false;

    if (!mapped) {
        sys_map_physical_region(queue->notify_reg, CURENVID, (uint64_t *)queue->notify_reg, sizeof(uint64_t), PROT_RW | PROT_CD);
        mapped = true;
    }

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

void
process_queue(struct virtq *queue, bool incoming) {
    size_t tail = queue->used_tail;
    size_t const mask = ~-(1 << queue->log2_size);
    size_t const done_idx = queue->used.idx;

    do {
        struct virtq_used_elem *used = &queue->used.ring[tail & mask];
        uint16_t id = used->id;

        if (incoming) {
            analyze_packet(queue, id);
        }

        unsigned freed_count = 1;

        uint16_t end = id;
        while (queue->desc[end].flags & VIRTQ_DESC_F_NEXT) {
            end = queue->desc[end].next;
            ++freed_count;
        }

        queue->desc[end].next = queue->desc_first_free;
        queue->desc_first_free = id;
        queue->desc_free_count += freed_count;
    } while ((++tail & 0xFFFF) != done_idx);

    queue->used_tail = tail;
}