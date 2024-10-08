#include "ethernet.h"
#include "connection.h"
#include "inc/stdio.h"
#include "net.h"
#include "queue.h"

static int process_ipv4_packet(struct ipv4_hdr_t *packet);

int
process_packet(struct virtio_net_hdr *net_hdr) {
    struct ethernet_pkt_t *base_pkt = (struct ethernet_pkt_t *)(net_hdr + 1);
    int res = 1;

    switch (ntohs(base_pkt->hdr.len_ethertype)) {
    case ETHERTYPE_ARP:
        if (trace_net)
            cprintf("ARP received\n");
        process_arp_packet((struct arp_packet_t *)base_pkt);
        break;

    case ETHERTYPE_IPv4:
        if (trace_net)
            cprintf("ipv4 received\n");
        res = process_ipv4_packet((struct ipv4_hdr_t *)(base_pkt));
        break;

    case 0:
        if (trace_net)
            cprintf("Broken ethernet frame (invalid type [invalid desc maybe?])\n");
        break;

    default:
        break;
    }

    // Снова все тот же костыль с очередью и ее невнятным GC, сука, как же я заебался
    base_pkt->hdr.len_ethertype = 0;
    return res;
}

static int
process_ipv4_packet(struct ipv4_hdr_t *packet) {
    switch (packet->protocol) {
    case IPV4_PROTO_TCP:
        return process_tcp_packet((struct tcp_hdr_t *)packet);

    case IPV4_PROTO_UDP:
        if (trace_net)
            cprintf("[netd]: unexpected udp packet\n");
        break;

    case IPV4_PROTO_ICMP:
        if (trace_net)
            cprintf("[netd]: unexpected icmp packet\n");
        break;

    default:
        if (trace_net)
            cprintf("[netd]: UNKNOWN ipv4 proto %d\n", packet->protocol);
    }
    return NO_PACKETS;
}