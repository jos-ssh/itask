/**
 * @file pci.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-08
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_PCI_PCI_H
#define __KMOD_PCI_PCI_H

#include <inc/acpi.h>
#include <inc/types.h>
#include <inc/list.h>
#include <inc/env.h>

#define MAX_PCI_BUSES 16
#define MAX_PCI_DEV_PER_BUS 32
#define MAX_PCI_FN_PER_DEV 8
#define MAX_PCI_DEVS (MAX_PCI_DEV_PER_BUS * MAX_PCI_FN_PER_DEV * MAX_PCI_BUSES)

#define PCI_VENDOR_INVALID 0xFFFF

#define PCI_COMMAND_BUS_MASTER 0x4
#define PCI_COMMAND_IO_SPACE 0x1

#define PCI_HEADER_TYPE_GENERAL        0x0
#define PCI_HEADER_TYPE_PCI_TO_PCI     0x1
#define PCI_HEADER_TYPE_PCI_TO_CARDBUS 0x2

#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80
#define PCI_HEADER_TYPE_MASK          0x7F

#define PCI_HEADER_TYPE_CHECK(header_type, type) \
    (((header_type)&PCI_HEADER_TYPE_MASK) == (type))

#define PCI_HEADER_TYPE_IS_MULTIFUNCTION(header_type) \
    (((header_type)&PCI_HEADER_TYPE_MULTIFUNCTION) != 0)

#define PCI_CLASS_BRIDGE 0x6
#define PCI_CLASS_SERIAL 0xC

#define PCI_SUBCLASS_PCI_TO_PCI 0x4
#define PCI_SUBCLASS_USB        0x3

#define PCI_PROG_IF_UHCI 0x00
#define PCI_PROG_IF_OHCI 0x10
#define PCI_PROG_IF_EHCI 0x20
#define PCI_PROG_IF_XHCI 0x30

#define PCI_BAR_MEMORY32_MASK   0xFFFFFFF0
#define PCI_BAR_MEMORY64_LOMASK 0xFFFFFFF0
#define PCI_BAR_MEMORY64_HIMASK 0xFFFFFFFF
#define PCI_BAR_IO_MASK         0xFFFFFFFC

#define PCI_BAR_TYPE_MASK   0x1
#define PCI_BAR_TYPE_MEMORY 0x0
#define PCI_BAR_TYPE_IO     0x1

#define PCI_BAR_CHECK_TYPE(bar, type) \
    (((bar)&PCI_BAR_TYPE_MASK) == (type))

#define PCI_BAR_MEMORY_TYPE_MASK  0x6
#define PCI_BAR_MEMORY_TYPE_32BIT 0x0
#define PCI_BAR_MEMORY_TYPE_64BIT 0x2

#define PCI_BAR_IS_IO(bar) (((bar)&PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_IO)

#define PCI_BAR_IS_MEMORY32(bar)                           \
    ((((bar)&PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_MEMORY) && \
     ((((bar)&PCI_BAR_MEMORY_TYPE_MASK) >> 1) == PCI_BAR_MEMORY_TYPE_32BIT))

#define PCI_BAR_IS_MEMORY64(bar)                           \
    ((((bar)&PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_MEMORY) && \
     ((((bar)&PCI_BAR_MEMORY_TYPE_MASK) >> 1) == PCI_BAR_MEMORY_TYPE_64BIT))

#define PCI_BAR_IO_GET_ADDR(bar) ((bar)&PCI_BAR_IO_MASK)

#define PCI_BAR_MEMORY32_GET_ADDR(bar) ((bar)&PCI_BAR_MEMORY32_MASK)

#define PCI_BAR_MEMORY64_GET_ADDR(lobar, hibar)        \
    ((((uint64_t)(lobar)) & PCI_BAR_MEMORY64_LOMASK) | \
     ((((uint64_t)(hibar)) & PCI_BAR_MEMORY64_HIMASK) << 32))

#define PCI_DEVICE(devfn)   ((devfn) >> 3)
#define PCI_FUNCTION(devfn) ((devfn)&0x7)
#define PCI_DEVFN(device, function) \
    ((((device) & (0x1F)) << 3) | ((function)&0x7))

#pragma pack(push, 1)

struct PciHeader {
    uint32_t vendor_id : 16;
    uint32_t device_id : 16;

    uint32_t command_reg : 16;
    uint32_t status_reg : 16;

    uint32_t revision_id : 8;
    uint32_t prog_if : 8;
    uint32_t subclass_code : 8;
    uint32_t class_code : 8;

    uint32_t cache_line_size : 8; // Legacy in PCIe
    uint32_t latency_timer : 8;   // Legacy in PCIe
    uint32_t header_type : 8;
    uint32_t bist : 8;
};

struct PciHeaderGeneral {
    struct PciHeader header;

    uint32_t bar[6];

    uint32_t cardbus_cis_ptr;

    uint32_t subsystem_vendor_id : 16;
    uint32_t subsystem_id : 16;

    uint32_t expansion_rom_base_addr;

    uint32_t capabilities_ptr : 8;
    uint32_t rsvd0 : 24;

    uint32_t rsvd1;

    uint32_t interrupt_line : 8;
    uint32_t interrupt_pin : 8;
    uint32_t min_grant : 8;   // Legacy in PCIe
    uint32_t max_latency : 8; // Legacy in PCIe
};

struct PciHeaderPciToPciBridge {
    struct PciHeader header;

    uint32_t bar[2];

    uint32_t primary_bus_num : 8;         // Legacy in PCIe
    uint32_t secondary_bus_num : 8;       // Lowest accessible bus number
    uint32_t subordinate_bus_num : 8;     // Highest accessible bus number
    uint32_t secondary_latency_timer : 8; // Legacy in PCIe

    uint32_t io_base : 8;
    uint32_t io_limit : 8;
    uint32_t secondary_status : 16;

    uint32_t memory_base : 16;
    uint32_t memory_limit : 16;

    uint32_t prefetch_memory_base : 16;
    uint32_t prefetch_memory_limit : 16;

    uint32_t prefetch_base_hi32;

    uint32_t prefetch_limit_hi32;

    uint32_t io_base_hi16 : 16;
    uint32_t io_limit_hi16 : 16;

    uint32_t capability_ptr : 8;
    uint32_t rsvd : 24;

    uint32_t expansion_rom_base_addr;

    uint32_t interrupt_line : 8;
    uint32_t interrupt_pin : 8;
    uint32_t bridge_control : 16;
};

typedef struct
{
    uint64_t base_address;
    uint16_t group_number;
    uint8_t start_pci_bus_number;
    uint8_t end_pci_bus_number;
    uint32_t rsvd;
} ECAMAddress;

typedef struct {
    ACPISDTHeader header;
    uint64_t rsvd;
    ECAMAddress groups[];
} MCFG;

#pragma pack(pop)

struct PciBus;

struct PciDevice {
    struct List device_list_node;
    struct List child_list_node;

    struct PciBus* bus;

    uint8_t devfn;

    struct PciHeader* header;
};

struct PciBus {
    struct List bus_list_node;
    struct List child_list_node;

    struct List devices;
    struct List buses;

    struct PciDevice* as_device;

    uint8_t bus_id;
};

extern envid_t g_InitdEnvid;

void pci_init(void);

void dump_pci_tree(void);

physaddr_t pci_device_get_memory_area(struct PciDevice* device, uint8_t bar_id, size_t* size);

uint32_t pci_device_get_io_base(struct PciDevice* device, uint8_t bar_id);

struct PciDevice* find_device(uint8_t class_code, uint8_t subclass_code,
                              uint8_t prog_if);


#endif /* pci.h */
