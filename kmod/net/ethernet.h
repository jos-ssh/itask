#pragma once
#include <stdint.h>
#include <sys/cdefs.h>

// Predefined values

static const uint8_t IP_ADRESS[4] = {10, 0, 2, 15};

// IEEE802.3 Ethernet Frame

// 14 bytes
struct __attribute__((__packed__)) ethernet_hdr_t {
    uint8_t d_mac[6];
    uint8_t s_mac[6];
    uint16_t len_ethertype;
    // ... followed by 64 to 1500 byte payload
    // ... followed by 32 bit CRC
};

// 1518 bytes
struct __attribute__((__packed__)) ethernet_pkt_t {
    // Header
    struct ethernet_hdr_t hdr;

    // 1500 byte packet plus room for CRC
    uint8_t packet[1500 + sizeof(uint32_t)];
};

struct __attribute__((__packed__)) arp_packet_t {
    struct ethernet_hdr_t eth_hdr;

    // Ethernet = 1
    uint16_t htype;

    // ETHERTYPE_IPv4
    uint16_t ptype;

    // Ethernet = 6
    uint8_t hlen;

    // IPv4 = 4
    uint8_t plen;

    // 1 = request, 2 = reply
    uint16_t oper;

    // Source MAC
    uint8_t source_mac[6];

    // Source IPv4 address
    uint8_t sender_ip[4];

    // Target MAC
    uint8_t target_mac[6];

    // Target IPv4 address
    uint8_t target_ip[4];
};

struct __attribute__((__packed__)) ipv4_hdr_t {
    struct ethernet_hdr_t eth_hdr;
    union {
        struct {
            uint8_t header_len : 4;
            uint8_t version : 4;
        };
        uint8_t ver_ihl;
    };

    uint8_t dscp_ecn; // QoS, not needed
    uint16_t len;
    uint16_t id;
    uint16_t flags_fragofs;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t hdr_checksum;
    uint8_t s_ip[4];
    uint8_t d_ip[4];
};

struct __attribute__((__packed__)) tcp_hdr_t {
    struct ipv4_hdr_t ipv4_hdr;

    /*
     * TCP header.
     * Per RFC 793, September, 1981.
     */
    uint16_t th_sport;  /* source port */
    uint16_t th_dport;  /* destination port */
    uint32_t th_seq;    /* sequence number */
    uint32_t th_ack;    /* acknowledgement number */
    uint8_t th_x2 : 4,  /* (unused) */
            th_off : 4; /* data offset */
    uint8_t th_flags;
#define TH_FIN         0x01
#define TH_SYN         0x02
#define TH_RST         0x04
#define TH_PUSH        0x08
#define TH_ACK         0x10
#define TH_URG         0x20
#define TH_ECE         0x40
#define TH_CWR         0x80
#define TH_FLAGS       (TH_FIN | TH_SYN | TH_RST | TH_PUSH | TH_ACK | TH_URG | TH_ECE | TH_CWR)
#define PRINT_TH_FLAGS "\20\1FIN\2SYN\3RST\4PUSH\5ACK\6URG\7ECE\10CWR"

    uint16_t th_win; /* window */
    uint16_t th_sum; /* checksum */
    uint16_t th_urp; /* urgent pointer */
};

#define ETHERTYPE_IPv4    0x0800
#define ETHERTYPE_ARP     0x0806
#define ETHERTYPE_WOL     0x0842
#define ETHERTYPE_RARP    0x8035
#define ETHERTYPE_IPv6    0x86DD
#define ETHERTYPE_FLOWCTL 0x8808

#define IPV4_PROTO_ICMP 0x01
#define IPV4_PROTO_TCP  0x06
#define IPV4_PROTO_UDP  0x11

//--------------------------
//  Byte ordering helpers
//--------------------------

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint32_t
htonl(uint32_t __hostlong) {
    return __builtin_bswap32(__hostlong);
}

static inline uint16_t
htons(uint16_t __hostshort) {
    return __builtin_bswap16(__hostshort);
}

static inline uint32_t
ntohl(uint32_t __netlong) {
    return __builtin_bswap32(__netlong);
}

static inline uint16_t
ntohs(uint16_t __netshort) {
    return __builtin_bswap16(__netshort);
}

#else

static inline uint32_t
htonl(uint32_t hostlong) {
    return hostlong;
}

static inline uint16_t
htons(uint16_t hostshort) {
    return hostshort;
}

static inline uint32_t
ntohl(uint32_t netlong) {
    return netlong;
}

static inline uint16_t
ntohs(uint16_t netshort) {
    return netshort;
}

#endif

void process_arp_packet(struct arp_packet_t* packet);
int process_tcp_packet(struct tcp_hdr_t* packet);
void send_to(struct tcp_hdr_t* client, const char* data, unsigned long ndata);
