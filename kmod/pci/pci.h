#ifndef JOS_KERN_PCI_H
#define JOS_KERN_PCI_H

#include <inc/pci.h>
#include <inc/acpi.h>

#define MAX_PCI_BUSES 16
#define MAX_PCI_DEV_PER_BUS 32
#define MAX_PCI_FN_PER_DEV 8
#define MAX_PCI_DEVS (MAX_PCI_DEV_PER_BUS * MAX_PCI_FN_PER_DEV * MAX_PCI_BUSES)

#pragma pack(push, 1)

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

void pci_init(void);

void dump_pci_tree(void);

void* pci_device_get_memory_area(struct PciDevice* device, uint8_t bar_id);

uint32_t pci_device_get_io_base(struct PciDevice* device, uint8_t bar_id);

struct PciDevice* find_device(uint8_t class_code, uint8_t subclass_code,
                              uint8_t prog_if);


#endif /* !JOS_KERN_PCI_H */
