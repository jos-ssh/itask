#include "ethernet.h"
#include "inc/lib.h"
#include "inc/mmu.h"
#include "inc/stdio.h"
#include <alloca.h>
#include <stdalign.h>
#include <string.h>
#include "net.h"
#include "queue.h"

struct __attribute__((__packed__)) SHITTY_PAYLOAD {
    struct virtio_net_hdr header;
    char _buffer[1550];
};

static alignas(PAGE_SIZE) struct SHITTY_PAYLOAD payload;

void process_arp_packet(struct arp_packet_t* arp) {
    cprintf("Sender=%02x:%02x:%02x:%02x:%02x:%02x"
                  " IP=%d.%d.%d.%d"
                  " Target=%02x:%02x:%02x:%02x:%02x:%02x"
                  " IP=%d.%d.%d.%d hlen=%d plen=%d htype=%d oper=%d\n",
                arp->source_mac[0],
                arp->source_mac[1],
                arp->source_mac[2],
                arp->source_mac[3],
                arp->source_mac[4],
                arp->source_mac[5],
                arp->sender_ip[0],
                arp->sender_ip[1],
                arp->sender_ip[2],
                arp->sender_ip[3],
                arp->target_mac[0],
                arp->target_mac[1],
                arp->target_mac[2],
                arp->target_mac[3],
                arp->target_mac[4],
                arp->target_mac[5],
                arp->target_ip[0],
                arp->target_ip[1],
                arp->target_ip[2],
                arp->target_ip[3],
                arp->hlen,
                arp->plen,
                ntohs(arp->htype),
                ntohs(arp->oper));

    if (memcmp(IP_ADRESS, arp->target_ip, sizeof(IP_ADRESS)) != 0) {
        cprintf("ARP to unknown address\n");
        return;
    }

    struct arp_packet_t* reply = (struct arp_packet_t *)payload._buffer;
    // Ethernet
    reply->htype = htons(1);
    // IPv4
    reply->ptype = htons(ETHERTYPE_IPv4);
    // 6 byte MAC
    reply->hlen = 6;
    // 4 byte IP
    reply->plen = 4;
    // Reply
    reply->oper = htons(2);   

    // Reply to sender's IP
    memcpy(reply->sender_ip, arp->target_ip, 4);
    // From this IP
    memcpy(reply->target_ip, arp->sender_ip, 4);

    reply->eth_hdr.len_ethertype = htons(ETHERTYPE_ARP);

    memcpy(reply->eth_hdr.s_mac, net.conf->mac, 6); // UNTRUSTED LINE
    memcpy(reply->source_mac, reply->eth_hdr.s_mac, 6); 

    // Broadcast destination
    memset(reply->eth_hdr.d_mac, 0xFF, 6);

    // To them
    memcpy(reply->target_mac, arp->source_mac, 6);

    struct virtq_desc* desc = alloc_desc(&net.sendq, 0);
 
    desc->len = 300;
    desc->addr = get_phys_addr(&payload);
    desc->flags = VIRTIO_NET_HDR_GSO_NONE;

    queue_avail(&net.sendq, 1);
    notify_queue(&net.sendq);
}