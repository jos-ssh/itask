#include "ethernet.h"
#include "inc/stdio.h"
#include "net.h"
#include "queue.h"

void process_packet(struct virtq *queue, uint64_t indx) {
    struct virtio_net_hdr* net_hdr = (struct virtio_net_hdr *) reverse_recv_buffer_addr(indx);
    // struct virtq_desc* desc = &queue->desc[indx];

    cprintf(" PACKET \n");
    struct ethernet_pkt_t* base_pkt = (struct ethernet_pkt_t*) (net_hdr + 1);

    switch (ntohs(base_pkt->hdr.len_ethertype)) {
        case ETHERTYPE_ARP:
            cprintf("ARP received\n");
            process_arp_packet((struct arp_packet_t *)base_pkt);
            break;
        
        case ETHERTYPE_IPv4:
            cprintf("ipv4 received\n");
            break;

        default:
            cprintf("unsupported packet\n");
            break;
    }

}