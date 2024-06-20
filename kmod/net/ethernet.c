#include "ethernet.h"
#include "inc/stdio.h"
#include "net.h"
#include "queue.h"
#include <stdint.h>

static void process_ipv4_packet(struct ipv4_hdr_t *packet);

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
            process_ipv4_packet((struct ipv4_hdr_t *)(base_pkt));
            break;

        case 0:
            cprintf("GC broken again\n");
            break;

        default:
            break;
    }

    // Снова все тот же костыль с очередью и ее невнятным GC, сука, как же я заебался
    base_pkt->hdr.len_ethertype = 0;
}

static void process_ipv4_packet(struct ipv4_hdr_t *packet) {
    switch (packet->protocol) {
        case IPV4_PROTO_TCP:
            process_tcp_packet((struct tcp_hdr_t *)packet);
            break;

        case IPV4_PROTO_UDP:
            cprintf("unexpected udp packet\n");
            break;

        case IPV4_PROTO_ICMP:
            cprintf("unexpected icmp packet\n");
            break;

        default:
            cprintf("UNKNOWN ipv4 proto %d\n", packet->protocol);
    }
}