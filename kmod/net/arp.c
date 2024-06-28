#include "ethernet.h"
#include "inc/stdio.h"
#include <inc/string.h>
#include "net.h"
#include "queue.h"

void process_arp_packet(struct arp_packet_t* arp) {
    if (memcmp(IP_ADRESS, arp->target_ip, sizeof(IP_ADRESS)) != 0) {
        cprintf("[netd]: ARP to unknown address\n");
        return;
    }

    struct virtio_packet_t* packet = allocate_virtio_packet();
    struct arp_packet_t* reply = (struct arp_packet_t *)(&packet->data);
    
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

    memcpy(reply->eth_hdr.s_mac, net->conf->mac, 6);
    memcpy(reply->source_mac, reply->eth_hdr.s_mac, 6); 

    // Broadcast destination
    memset(reply->eth_hdr.d_mac, 0xFF, 6);

    // To them
    memcpy(reply->target_mac, arp->source_mac, 6);

    send_virtio_packet(packet);
}