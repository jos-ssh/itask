#include "connection.h"
#include "ethernet.h"
#include "inc/stdio.h"
#include "inc/string.h"
#include "queue.h"
#include <inc/assert.h>
#include <stdatomic.h>
#include <inc/types.h>
#include "net.h"

static uint32_t in_num = 0;
static uint32_t out_num = 1;

static uint16_t
ipv4_checksum(struct ipv4_hdr_t* hdr) {
    hdr->hdr_checksum = 0;
    uint16_t const* in = (uint16_t*)(((char*)hdr) + sizeof(struct ethernet_hdr_t));

    uint32_t total = 0;
    for (size_t i = 0, e = (hdr->ver_ihl & 0xF) << 1;
         i < e; ++i) {
        total += htons(in[i]);
    }

    total += total >> 16;
    total &= 0xFFFF;

    return htons(~total);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

static void
tcp_checksum(struct tcp_hdr_t* pkt) {
    uint64_t sum = 0;
    unsigned short tcpLen = ntohs(pkt->ipv4_hdr.len) - (pkt->ipv4_hdr.header_len << 2);

    // add the pseudo header
    // the source & dest ip
    uint16_t* addr_seg = (uint16_t*)&pkt->ipv4_hdr.s_ip;
    for (int i = 0; i < 4; ++i) {
        sum += addr_seg[i]; // & 0xFFFF;
    }

    // protocol and reserved: 6
    sum += htons(IPV4_PROTO_TCP);
    // the length
    sum += htons(tcpLen);

    uint16_t* payload = (uint16_t*)(&pkt->th_sport);

    // add the IP payload
    // initialize checksum to 0
    pkt->th_sum = 0;
    while (tcpLen > 1) {
        sum += *payload++;
        tcpLen -= 2;
    }

    // if any bytes left, pad the bytes and add
    if (tcpLen > 0) {
        sum += ((*payload) & htons(0xFF00));
    }

    // Fold 32-bit sum to 16 bits: add carrier to result
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;

    // set computation result
    pkt->th_sum = (unsigned short)sum;
}

#pragma GCC diagnostic pop

void
fill_reply_to(struct tcp_hdr_t* reply, const struct tcp_hdr_t* in) {
    // ethernet
    reply->ipv4_hdr.eth_hdr.len_ethertype = htons(ETHERTYPE_IPv4);
    memcpy(reply->ipv4_hdr.eth_hdr.s_mac, net->conf->mac, 6);
    memcpy(reply->ipv4_hdr.eth_hdr.d_mac, in->ipv4_hdr.eth_hdr.s_mac, 6);

    // ipv4
    memcpy(reply->ipv4_hdr.s_ip, IP_ADRESS, 4);
    memcpy(reply->ipv4_hdr.d_ip, in->ipv4_hdr.s_ip, 4);
    reply->ipv4_hdr.version = 4;
    reply->ipv4_hdr.header_len = (sizeof(struct ipv4_hdr_t) - sizeof(struct ethernet_hdr_t)) / 4;
    reply->ipv4_hdr.len = htons(sizeof(struct tcp_hdr_t) - sizeof(struct ethernet_hdr_t)); // Can be increased later
    reply->ipv4_hdr.id = 0;
    reply->ipv4_hdr.flags_fragofs = 0;
    reply->ipv4_hdr.ttl = 64;
    reply->ipv4_hdr.protocol = IPV4_PROTO_TCP;
    reply->ipv4_hdr.hdr_checksum = ipv4_checksum(&reply->ipv4_hdr);

    // tcp
    reply->th_dport = in->th_sport;
    reply->th_sport = in->th_dport;

    reply->th_ack = htonl(in_num);
    reply->th_seq = htonl(out_num);
    reply->th_off = (sizeof(struct tcp_hdr_t) - sizeof(struct ipv4_hdr_t)) / 4;
    reply->th_win = htons(65535); // ?
}

static int
reply_to_syn(struct tcp_hdr_t* syn) {
    struct virtio_packet_t* reply_packet = allocate_virtio_packet();
    struct tcp_hdr_t* reply = (struct tcp_hdr_t*)(&reply_packet->data);

    fill_reply_to(reply, syn);
    reply->th_ack = htonl(ntohl(reply->th_ack) + 1);
    out_num++;

    reply->th_flags |= TH_ACK | TH_SYN;
    tcp_checksum(reply);
    send_virtio_packet(reply_packet);
    return CONNECTION_PROCESSED;
}

static int
reply_to_fin(struct tcp_hdr_t* fin) {
    struct virtio_packet_t* reply_packet = allocate_virtio_packet();
    struct tcp_hdr_t* reply = (struct tcp_hdr_t*)(&reply_packet->data);

    fill_reply_to(reply, fin);

    reply->th_flags = TH_RST;
    tcp_checksum(reply);
    send_virtio_packet(reply_packet);
    return CONNECTION_PROCESSED;
}

static char*
get_payload(struct tcp_hdr_t* packet) {
    return ((char*)&packet->ipv4_hdr) + sizeof(struct ethernet_hdr_t) + (packet->ipv4_hdr.header_len << 2u) + (packet->th_off << 2u);
}

size_t
get_payload_size(struct tcp_hdr_t* packet) {
    uint32_t size = packet->ipv4_hdr.len - htons(sizeof(struct tcp_hdr_t) - sizeof(struct ethernet_hdr_t));
    return ntohs(size);
}


void
send_to(struct tcp_hdr_t* client, const char* data, size_t ndata) {
    struct virtio_packet_t* reply_packet = allocate_virtio_packet();
    struct tcp_hdr_t* reply = (struct tcp_hdr_t*)(&reply_packet->data);

    fill_reply_to(reply, client);

    reply->th_flags = TH_ACK | TH_PUSH;

    memcpy(get_payload(reply), data, ndata);
    out_num += ndata;

    reply->ipv4_hdr.len += htons(ndata);
    reply->ipv4_hdr.hdr_checksum = ipv4_checksum(&reply->ipv4_hdr);

    tcp_checksum(reply);
    send_virtio_packet(reply_packet);
}

void
echo_to(struct tcp_hdr_t* packet) {
    char buf[1000];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, "Server: ");
    strcat(buf, get_payload(packet));

    send_to(packet, buf, strlen(buf));
}


int
process_tcp_packet(struct tcp_hdr_t* packet) {
    if (trace_net && ntohl(packet->th_seq) < in_num) {
        cprintf("Runaway detected, %u with remembered %u [possible retransmission]", ntohl(packet->th_seq), in_num);
    }

    if (trace_net)
        cprintf("Received tcp packet to port %d from port %d\n", ntohs(packet->th_dport), ntohs(packet->th_sport));

    in_num = ntohl(packet->th_seq);
    size_t packet_size = ntohs(packet->ipv4_hdr.len) - (packet->ipv4_hdr.header_len << 2u) - (packet->th_off << 2u);

    // Is this SYN only?
    if ((packet->th_flags & TH_SYN) && !(packet->th_flags & TH_ACK)) {
        return reply_to_syn(packet);
    }

    // Just ACK from client... let it pass
    if (packet->th_flags == TH_ACK && packet_size == 0) {
        if (atomic_load(&g_Connection.state) == kCreated) {
            memcpy(&g_Connection.client, packet, sizeof(*packet));
            return CONNECTION_ESTABLISHED;
        }
        return SEND_PROCESSED;
    }

    // le FIN
    if (packet->th_flags & TH_FIN) {
        reply_to_fin(packet);
        return CONNECTION_CLOSED;
    }


    // TODO: remove, debug only
    // echo_to(packet);

    char* data = get_payload(packet);
    size_t n = get_payload_size(packet);
    write_buf(&g_Connection.recieve_buf, (const unsigned char*)data, n);

    struct virtio_packet_t* reply_packet = allocate_virtio_packet();
    struct tcp_hdr_t* reply = (struct tcp_hdr_t*)(&reply_packet->data);

    in_num += packet_size;

    // send ACK to client
    fill_reply_to(reply, packet);

    reply->th_flags = TH_ACK;
    tcp_checksum(reply);
    send_virtio_packet(reply_packet);

    return RECIEVE_PROCESSED;
}