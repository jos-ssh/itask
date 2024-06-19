/* ### Static declarations ####
*/
// #include "inc/stdio.h"
#include "inc/memlayout.h"
#include "inc/pool_alloc.h"
#include "inc/stdio.h"
#include "net.h"
#include "inc/assert.h"
#include "inc/kmod/pci.h"
#include "inc/mmu.h"
#include <inc/lib.h>
#include <inc/rpc.h>
#include <stdint.h>
#include <string.h>
#include <fs/pci_classes.h>
#include <kmod/pci/pci.h>
#include <inc/kmod/init.h>
#include "queue.h"
#include "kern/pci_common.h"

static envid_t
find_module(envid_t initd, const char* name);

static void
setup_device(struct PciHeaderGeneral*);

struct recv_buffer {
    _Alignas(PAGE_SIZE) char _[PAGE_SIZE];
};

struct recv_buffer recv_buffers[VIRTQ_SIZE];
struct virtio_packet_t send_buffers[SEND_BUF_NUM];

void *reverse_recv_buffer_addr(int64_t index) {
    return &recv_buffers[index]; // для sendq там indx -> indx;
}

/* #### Initialization ####
*/

void initialize() {
    g_PcidEnvid = find_module(g_InitdEnvid, PCID_MODNAME);
    uint32_t device_id = pcid_device_id(PCI_CLASS_NETWORK, PCI_SUBCLASS_NETWORK_ETHERNET, 0); 

    union PcidResponse* response = (void*) RECEIVE_ADDR;
    int res = rpc_execute(g_PcidEnvid, PCID_REQ_FIND_DEVICE | device_id, NULL, (void **)&response);
    UNWRAP(res, "Failed to find virtio-net");

    struct PciHeaderGeneral* pci_header = (struct PciHeaderGeneral*)response->dev_confspace;

    // map conf space
    res = sys_map_physical_region(response->dev_confspace, CURENVID, pci_header, PAGE_SIZE, PROT_RW | PROT_CD);
    UNWRAP(res, "Failed to map conf space");

    // Map MMIO
    res = sys_map_physical_region(0x800000000, CURENVID, (void*)0x800000000, 0x4000, PROT_RW | PROT_CD);
    UNWRAP(res, "Failed to map MMIO");

    setup_device(pci_header);

    // Setup allocator for send buffers
    net.alloc = MAKE_ALLOCATOR(send_buffers);
    pool_allocator_init(&net.alloc);

    g_IsNetdInitialized = true;
} 

static bool
is_bar_mmio(uint32_t *base_addrs, size_t bar) {
    return PCI_BAR_RTE_GET(base_addrs[bar]) == 0;
}

static bool
is_bar_64bit(uint32_t *base_addrs, size_t bar) {
    return (PCI_BAR_RTE_GET(base_addrs[bar]) == 0) &&
           (PCI_BAR_MMIO_TYPE_GET(base_addrs[bar]) == PCI_BAR_MMIO_TYPE_64BIT);
}

static uint64_t
get_bar(uint32_t *base_addrs, size_t bar) {
    uint64_t addr;

    if (is_bar_mmio(base_addrs, bar)) {
        addr = base_addrs[bar] & PCI_BAR_MMIO_BA;

        if (is_bar_64bit(base_addrs, bar)) {
            addr |= ((uint64_t)(base_addrs[bar + 1])) << 32;
        }
    } else {
        // Mask off low 2 bits
        addr = base_addrs[bar] & -4;
    }

    return addr;
}

static void
parse_common_cfg(volatile struct virtio_pci_common_cfg_t *cfg_header) {
    // Reset device
    cfg_header->device_status = 0;

    // Wait until reset completes
    while (cfg_header->device_status != 0) {
        asm volatile("pause");
    }

    // Set ACK bit (we recognised this device)
    cfg_header->device_status |= VIRTIO_STATUS_ACKNOWLEDGE;

    // Set DRIVER bit (we have driver for this device)
    cfg_header->device_status |= VIRTIO_STATUS_DRIVER;

    // Accept features
    // VIRTIO_NET_F_MAC — device can tell mac address
    // VIRTIO_NET_F_STATUS — device can tell link status
    // (not used) VIRTIO_NET_F_MRG_RXBUF — device can split package into multiple descriptors
    for (int i = 0; i < 4; ++i) {
        cfg_header->device_feature_select = i;
        (void)cfg_header->device_feature;
        cfg_header->driver_feature_select = i;
        if (i == 0) {
            cfg_header->driver_feature = VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS;
        } else {
            cfg_header->driver_feature = 0x0;
        }
    }

    // Say that we are ready to check featurus
    cfg_header->device_status |= VIRTIO_STATUS_FEATURES_OK;

    // Check if they are OK with this feature set
    if (!(cfg_header->device_status & VIRTIO_STATUS_FEATURES_OK)) {
        cfg_header->device_status |= VIRTIO_STATUS_FAILED;
        panic("FAILED TO SETUP NETWORK: Feature error");
    }

    // Config two queues
    net.sendq.queue_idx = VIRTIO_SENDQ;
    setup_queue(&net.sendq, cfg_header);

    net.recvq.queue_idx = VIRTIO_RECVQ;
    setup_queue(&net.recvq, cfg_header);

    cfg_header->queue_select = VIRTIO_SENDQ;

    for (int i = 0; i < VIRTQ_SIZE; ++i) {
        recv_buffers[i]._[0] = 0; // force alloc
        net.recvq.desc[i].addr = get_phys_addr(recv_buffers + i);
        net.recvq.desc[i].len  = sizeof(struct recv_buffer);
        net.recvq.desc[i].flags |= VIRTQ_DESC_F_WRITE;
    }

    queue_avail(&net.recvq, VIRTQ_SIZE);
    notify_queue(&net.recvq);

    // Set DRIVER_OK flag
    cfg_header->device_status |= VIRTIO_STATUS_DRIVER_OK;
}

static void
setup_device(struct PciHeaderGeneral* header) {
    assert(header->header.vendor_id == VENDOR_ID && header->header.device_id == DEVICE_ID);

    uint8_t cap_offset = header->capabilities_ptr;
    uint8_t cap_offset_old = cap_offset;
    uint64_t notify_cap_size = 0;
    uint32_t notify_cap_offset = 0;
    uint32_t notify_off_multiplier = 0;

    while (cap_offset != 0) {
        struct pci_cap_hdr_t cap_header;
        cap_header.cap_vendor = 0;

        // Search for vendor specific header
        while (cap_header.cap_vendor != PCI_CAP_VENDSPEC) {
            memcpy( (uint8_t *)&cap_header, (char*)header + cap_offset, sizeof(cap_header));
            cap_offset_old = cap_offset;
            cap_offset = cap_header.cap_next;
        }

        uint64_t addr;
        uint64_t bar_addr;

        struct virtio_pci_common_cfg_t *common_cfg_ptr;

        switch (cap_header.type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            bar_addr = get_bar(header->bar, cap_header.bar);
            addr = cap_header.offset + bar_addr;
            common_cfg_ptr = (struct virtio_pci_common_cfg_t *)addr;
            
            // RECVQ notify
            common_cfg_ptr->queue_select = VIRTIO_RECVQ;
            net.recvq.notify_reg += notify_cap_offset + common_cfg_ptr->queue_notify_off * notify_off_multiplier;
            
            // SENDQ notify
            common_cfg_ptr->queue_select = VIRTIO_SENDQ;
            net.sendq.notify_reg += notify_cap_offset + common_cfg_ptr->queue_notify_off * notify_off_multiplier;

            if (notify_cap_size < common_cfg_ptr->queue_notify_off * notify_off_multiplier + 2) {
                panic("Wrong size\n");
            }

            parse_common_cfg(common_cfg_ptr);
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            if (notify_cap_size) {
                break;
            }
            notify_cap_size = cap_header.length;

            notify_cap_offset = cap_header.offset;
            net.sendq.notify_reg += get_bar(header->bar, cap_header.bar);
            net.recvq.notify_reg += get_bar(header->bar, cap_header.bar);

            notify_off_multiplier = *(uint32_t *)(((char*) header) + cap_offset_old + sizeof(cap_header));
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            addr = cap_header.offset + get_bar(header->bar, cap_header.bar);
            net.isr_status = (uint8_t *)addr;
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            net.conf = (struct virtio_net_config_t *)(cap_header.offset + get_bar(header->bar, cap_header.bar));
            break;
        case VIRTIO_PCI_CAP_PCI_CFG: break;
        default: break;
        }

        cap_offset = cap_header.cap_next;
    }
}

/* #### Helper functions ####
*/

static envid_t
find_module(envid_t initd, const char* name) {
    static union InitdRequest request;

    request.find_kmod.max_version = -1;
    request.find_kmod.min_version = -1;
    strcpy(request.find_kmod.name_prefix, name);

    void* res_data = NULL;
    return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, &res_data);
}