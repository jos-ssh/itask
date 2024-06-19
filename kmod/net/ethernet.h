#pragma once
#include <byteswap.h>
#include <stdint.h>

// Predefined values

static const uint8_t IP_ADRESS[4] = {10, 0, 2, 15};

// IEEE802.3 Ethernet Frame

// 14 bytes
struct  __attribute__((__packed__)) ethernet_hdr_t {
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

#define ETHERTYPE_IPv4      0x0800
#define ETHERTYPE_ARP       0x0806
#define ETHERTYPE_WOL       0x0842
#define ETHERTYPE_RARP      0x8035
#define ETHERTYPE_IPv6      0x86DD
#define ETHERTYPE_FLOWCTL   0x8808


//--------------------------
//  Byte ordering helpers
//--------------------------

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint32_t htonl(uint32_t __hostlong)
{
    return bswap_32(__hostlong);
}

static inline uint16_t htons(uint16_t __hostshort)
{
    return bswap_16(__hostshort);
}

static inline uint32_t ntohl(uint32_t __netlong)
{
    return bswap_32(__netlong);
}

static inline uint16_t ntohs(uint16_t __netshort)
{
    return bswap_16(__netshort);
}

#else

static inline uint32_t htonl(uint32_t hostlong)
{
    return hostlong;
}

static inline uint16_t htons(uint16_t hostshort)
{
    return hostshort;
}

static inline uint32_t ntohl(uint32_t netlong)
{
    return netlong;
}

static inline uint16_t ntohs(uint16_t netshort)
{
    return netshort;
}

#endif


void process_arp_packet(struct arp_packet_t* packet);