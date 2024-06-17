#include "inc/mmu.h"
#include "inc/stdio.h"
#include "net.h"
#include "queue.h"
#include <stdint.h>

void analyze_packet(struct virtq *queue, size_t indx) {
    struct virtio_net_hdr* net_hdr = (struct virtio_net_hdr *) reverse_buffer_addr(indx);
    // struct virtq_desc* desc = &queue->desc[indx];

    cprintf(" PACKET \n");

    char* packet = (char *) (net_hdr + 1);
    size_t size = PAGE_SIZE - sizeof(*net_hdr);

    for (int i = 0; i < size;) {
        for (int j = 0; j < 8; ++j, ++i) {
            uint32_t a = (uint32_t)((unsigned char)(packet[i]));
            cprintf("%02X ", a);
        }
        cprintf("\n");
    }
}