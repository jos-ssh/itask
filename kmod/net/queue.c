#include "queue.h"
#include "inc/pool_alloc.h"
#include "inc/stdio.h"
#include "net.h"
#include <inc/lib.h>
#include <inc/string.h>

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

    queue->desc_free_count = VIRTQ_SIZE;
    for (int i = queue->desc_free_count; i > 0; --i) {
        queue->desc[i - 1].next = queue->desc_first_free;
        queue->desc_first_free = i - 1;
    }
}

void
notify_queue(struct virtq *queue) {
    *((uint64_t *)queue->notify_reg) = queue->queue_idx;
}

static uint16_t virtio_descr_add_buffer(struct virtq* virtqueue, void* buffer, bool writable) {
    uint16_t descr_free = virtqueue->desc_first_free;
    uint16_t ret_value  = descr_free; 

    uint16_t next_descr_free = 0;

    next_descr_free = (descr_free + 1) % VIRTQ_SIZE;
    virtqueue->desc[descr_free].len = sizeof(struct virtio_packet_t);
    virtqueue->desc[descr_free].addr = get_phys_addr(buffer);

    virtqueue->reverse_addr[descr_free] = buffer;

    if (writable) {
        virtqueue->desc[descr_free].flags |= VIRTQ_DESC_F_WRITE;
    }

    virtqueue->desc[descr_free].next = 0;
    descr_free = next_descr_free;

    virtqueue->desc_first_free = descr_free;
    --virtqueue->desc_free_count;

    return ret_value;
}

int virtio_snd_buffers(struct virtq *virtqueue, void* buffer, bool writable) {
    if (virtqueue->desc_free_count == 0){
        cprintf("Not enough space in descr table. virtio_snd_buffers() failed. \n");
        return -1;
    }

    uint16_t chain_head = virtio_descr_add_buffer(virtqueue, buffer, writable);
    
    uint16_t avail_ind = virtqueue->avail.idx % VIRTQ_SIZE;
    virtqueue->avail.ring[avail_ind] = chain_head;
    virtqueue->last_avail = avail_ind;

    virtqueue->avail.idx += 1;

    notify_queue(virtqueue);
    return 0;
}

void gc_sendq_queue() {
    struct virtq *sendq = &net->sendq;

    while (sendq->used_tail != sendq->used.idx) {
        uint16_t index = sendq->used_tail % VIRTQ_SIZE;

        struct virtq_used_elem* used_elem = &(sendq->used.ring[index]);
        uint16_t desc_idx = used_elem->id;

        // Free mem
        char* packet_ptr = (char *)sendq->reverse_addr[desc_idx];
        pool_allocator_free_object(&net->alloc, packet_ptr - offsetof(struct send_buffer_t, packet));

        uint16_t chain_len = 1;
        while (sendq->desc[desc_idx].flags & VIRTQ_DESC_F_NEXT) {
            chain_len += 1;
            desc_idx = sendq->desc[desc_idx].next;
        }

        sendq->desc_free_count  += chain_len;
        sendq->used_tail += 1;
    }
}

struct virtio_packet_t* allocate_virtio_packet() {
    struct send_buffer_t *buff = pool_allocator_alloc_object(&net->alloc);
    memset(&buff->packet, 0, sizeof(buff->packet));
    return &buff->packet;
}

void send_virtio_packet(struct virtio_packet_t* packet) {
    gc_sendq_queue();

    packet->vheader.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    virtio_snd_buffers(&net->sendq, packet, false);
}

int process_receive_queue(struct virtq *queue) {
    if (queue->used_tail == queue->used.idx) {
        return 0; // No new data arrived
    }

    uint16_t index = queue->used_tail % VIRTQ_SIZE;
    struct virtq_used_elem* used_elem = &(queue->used.ring[index]);
    uint16_t desc_idx = used_elem->id;

    void *buff_addr = queue->reverse_addr[desc_idx];
    int res = process_packet(buff_addr);

    queue->desc_free_count += 1;

    // Return buffers
    memset(buff_addr, 0, sizeof(struct recv_buffer));
    virtio_snd_buffers(queue, buff_addr, true);

    queue->used_tail += 1;
    return res;
}
