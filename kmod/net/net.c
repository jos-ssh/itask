#include "net.h"
#include "inc/assert.h"
#include "inc/kmod/pci.h"
#include "inc/mmu.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/kmod/init.h>
#include <string.h>
#include <fs/pci_classes.h>
#include <kmod/pci/pci.h>
#include "virtio.h"
#include "kern/pcireg.h"

#define UNWRAP(res, line) do { if (res != 0) { panic(line ": %i", res); } } while (0)

/* #### Globals ####
*/

envid_t g_InitdEnvid;
envid_t g_PcidEnvid;
bool g_IsNetdInitialized = false;

#define VENDOR_ID 0x1AF4
#define DEVICE_ID 0x1000

/* ### Static declarations ####
*/

static envid_t
find_module(envid_t initd, const char* name);

static void
setup_device(struct PciHeaderGeneral*);

/* #### Initialization ####
*/

void initialize() {
    cprintf("DEBUG: netd initialization\n");

    cprintf("INITD ENVID: %0x\n", g_InitdEnvid);
    g_PcidEnvid = find_module(g_InitdEnvid, PCID_MODNAME);
    cprintf("PCID ENVID: %0x\n", g_PcidEnvid);

    __auto_type device_id = pcid_device_id(PCI_CLASS_NETWORK, PCI_SUBCLASS_NETWORK_ETHERNET, 0); 

    union PcidResponse* response = (void*) RECEIVE_ADDR;
    int res = rpc_execute(g_PcidEnvid, PCID_REQ_FIND_DEVICE | device_id, NULL, (void **)&response);
    UNWRAP(res, "Failed to find virtio-net");

    struct PciHeaderGeneral* pci_header = (struct PciHeaderGeneral*)response->dev_confspace;

    // map conf space
    res = sys_map_physical_region(response->dev_confspace, CURENVID, pci_header, PAGE_SIZE, PROT_RW | PROT_CD);
    UNWRAP(res, "Failed to map conf space");

    setup_device(pci_header);

    g_IsNetdInitialized = true;
} 

static void
setup_device(struct PciHeaderGeneral* header) {
    assert(header->header.vendor_id == VENDOR_ID && header->header.device_id == DEVICE_ID);

    uint8_t cap_offset = header->capabilities_ptr;
    uint8_t cap_offset_old = cap_offset;
    // uint64_t notify_cap_size = 0;
    // uint32_t notify_cap_offset = 0;
    // uint32_t notify_off_multiplier = 0;

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
        uint64_t notify_reg;
        uint64_t bar_addr;

        volatile struct virtio_pci_common_cfg_t *common_cfg_ptr;

        switch (cap_header.type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            cprintf("COMMON\n");
            // bar_addr = get_bar(base_addrs, cap_header.bar);
            // addr = cap_header.offset + bar_addr;
            // common_cfg_ptr = (volatile struct virtio_pci_common_cfg_t *)addr;
            // notify_reg = notify_cap_offset + common_cfg_ptr->queue_notify_off * notify_off_multiplier;
            // gpu.controlq.notify_reg += notify_reg;

            // if (notify_cap_size < common_cfg_ptr->queue_notify_off * notify_off_multiplier + 2) {
            //     cprintf("Wrong size\n");
            // }
            // parse_common_cfg(pcif, common_cfg_ptr);
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            cprintf("NOTIFY\n");

            // if (notify_cap_size) {
            //     break;
            // }
            // notify_cap_size = cap_header.length;
            // notify_cap_offset = cap_header.offset;

            // gpu.controlq.notify_reg += get_bar(base_addrs, cap_header.bar);

            // pci_memcpy_from(pcif, cap_offset_old + sizeof(cap_header),
            //                 (uint8_t *)&notify_off_multiplier, sizeof(notify_off_multiplier));
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            cprintf("ISR\n");

            // addr = cap_header.offset + get_bar(base_addrs, cap_header.bar);
            // gpu.isr_status = (uint8_t *)addr;
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            cprintf("DEVICE\n");
            // gpu.conf = (struct virtio_gpu_config *)(cap_header.offset + get_bar(base_addrs, cap_header.bar));
            break;
        case VIRTIO_PCI_CAP_PCI_CFG: break;
        default: break;
        }

        cap_offset = cap_header.cap_next;
    }
}

/* #### I/O RPC
*/

void serve_teapot() {
    if (!g_IsNetdInitialized) {
        initialize();
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