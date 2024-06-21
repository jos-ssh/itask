#pragma once

#include <inc/types.h>
#include <inc/pool_alloc.h>
#include <stdint.h>
#include "ethernet.h"

#define VIRTQ_SIZE 64
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
/* The bit of the ISR which indicates a device configuration change. */
#define VIRTIO_PCI_ISR_CONFIG 0x2
#define VIRTIO_PCI_ISR_NOTIFY 0x1
#define VIRTIO_PCI_IRQ_CONFIG 0x1
#define DEFAULT_RESOURCE_ID   0x1

#define VIRTIO_RECVQ 0
#define VIRTIO_SENDQ 1

/* The device uses this in used->flags to advise the driver: don't kick me
 * when you add a buffer.  It's unreliable, so it's simply an
 * optimization. */
#define VIRTQ_USED_F_NO_NOTIFY 1
/* The driver uses this in avail->flags to advise the device: don't
 * interrupt me when you consume a buffer.  It's unreliable, so it's
 * simply an optimization.  */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

/* Support for indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC 28

/* Support for avail_event and used_event fields */
#define VIRTIO_F_EVENT_IDX 29

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT 27

// Device status
#define VIRTIO_STATUS_ACKNOWLEDGE   1
#define VIRTIO_STATUS_DRIVER        2
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_STATUS_FEATURES_OK   8
#define VIRTIO_STATUS_NEED_RESET    64
#define VIRTIO_STATUS_FAILED        128

// Feature bits
#define VIRTIO_NET_F_MAC            (1u << 5)
#define VIRTIO_NET_F_MRG_RXBUF      (1u << 15)
#define VIRTIO_NET_F_STATUS         (1u << 16)

// Common configuration
#define VIRTIO_PCI_CAP_COMMON_CFG   1

// Notifications
#define VIRTIO_PCI_CAP_NOTIFY_CFG   2

// ISR Status
#define VIRTIO_PCI_CAP_ISR_CFG      3

// Device specific configuration
#define VIRTIO_PCI_CAP_DEVICE_CFG   4

// PCI configuration access
#define VIRTIO_PCI_CAP_PCI_CFG      5

/* Virtqueue descriptors: 16 bytes.
 * These can chain together via "next". */
struct virtq_desc {
    /* Address (guest-physical). */
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTQ_SIZE];
    /* Only if VIRTIO_F_EVENT_IDX: */ /*... and we have it*/
    uint16_t used_event;
};

/* uint32_t is used here for ids for padding reasons. */
struct virtq_used_elem {
    /* Index of start of used descriptor chain. */
    uint32_t id;
    /* Total length of the descriptor chain which was written to. */
    uint32_t len;
};

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[VIRTQ_SIZE];
    /* Only if VIRTIO_F_EVENT_IDX: */
    uint16_t avail_event;
};

struct virtq {
    uint64_t notify_reg;
    uint32_t log2_size;
    uint32_t used_tail;
    uint32_t desc_first_free;
    uint32_t desc_free_count;
    uint64_t queue_idx;

    _Alignas(4096) struct virtq_desc desc[VIRTQ_SIZE];
    _Alignas(4096) struct virtq_avail avail;
    _Alignas(4096) struct virtq_used used;
};

struct virtio_pci_common_cfg_t {
    // About the whole device

    // read-write
    uint32_t device_feature_select;

    // read-only for driver
    uint32_t device_feature;

    // read-write
    uint32_t driver_feature_select;

    // read-write
    uint32_t driver_feature;

    // read-write
    uint16_t config_msix_vector;

    // read-only for driver
    uint16_t num_queues;

    // read-write
    uint8_t device_status;

    // read-only for driver
    uint8_t config_generation;

    // About a specific virtqueue

    // read-write
    uint16_t queue_select;

    // read-write, power of 2, or 0
    uint16_t queue_size;

    // read-write
    uint16_t queue_msix_vector;

    // read-write
    uint16_t queue_enable;

    // read-only for driver
    uint16_t queue_notify_off;

    // read-write
    uint64_t queue_desc;

    // read-write
    uint64_t queue_avail;

    // read-write
    uint64_t queue_used;
};

struct virtio_net_config_t {
    uint8_t mac[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
    uint32_t speed;
    uint8_t duplex;
    uint8_t rss_max_key_size;
    uint16_t rss_max_indirection_table_length;
    uint32_t supported_hash_types;
};

struct virtio_net_device_t {
    struct virtq sendq;
    struct virtq recvq;

    PoolAllocator alloc;

    uint8_t *isr_status;
    struct virtio_net_config_t *conf;
};

struct virtio_net_hdr {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1
#define VIRTIO_NET_HDR_F_DATA_VALID 2
#define VIRTIO_NET_HDR_F_RSC_INFO 4
    uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE 0
#define VIRTIO_NET_HDR_GSO_TCPV4 1
#define VIRTIO_NET_HDR_GSO_UDP 3
#define VIRTIO_NET_HDR_GSO_TCPV6 4
#define VIRTIO_NET_HDR_GSO_UDP_L4 5
#define VIRTIO_NET_HDR_GSO_ECN 0x80
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    // uint16_t num_buffers; // не будет, так как не согласовано склеивание буферов + мы легаси драйвер (хз почему...)
};

struct virtio_packet_t {
    // hw driver side (send_virtio_packet) 
    struct virtio_net_hdr vheader;
    // sf driver side (process_packet)
    struct ethernet_pkt_t data;
};

static inline int
virtq_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old_idx) {
    return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx);
}

void
setup_queue(struct virtq *queue, volatile struct virtio_pci_common_cfg_t *cfg_header);

void
notify_queue(struct virtq *queue);

void
queue_avail(struct virtq *queue, uint32_t count);

struct virtq_desc *
alloc_desc(struct virtq *queue, int writable);

void
process_queue(struct virtq *queue, bool incoming);

struct virtio_packet_t* allocate_virtio_packet();
void send_virtio_packet(struct virtio_packet_t* packet);