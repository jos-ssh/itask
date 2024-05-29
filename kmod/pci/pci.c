#include "pci.h"

#include <inc/pci.h>
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/list.h>

#include "mmio.h"

#ifndef trace_pci
#define trace_pci 0
#endif // !trace_pci

#define PCI_ECAM_SIZE 0x1000

#define PCI_DEV_PADDR(base, bus, dev, fn) \
    ((base) + ((bus) << 20) + ((dev) << 15) + ((fn) << 12))

// TODO: implement
static MCFG* get_mcfg(void);

static bool g_isPciInitialised;

static struct PciBus g_freeBuses[MAX_PCI_BUSES];
static struct PciDevice g_freeDevices[MAX_PCI_DEVS];

static struct List g_buses;
static size_t g_busCount = 0;

static struct List g_devices;
static size_t g_deviceCount = 0;

static struct List g_pciRoots;

struct PciDevice*
find_device(uint8_t class_code, uint8_t subclass_code, uint8_t prog_if) {
    if (!g_isPciInitialised) {
        pci_init();
    }

    struct PciDevice* returned = NULL;
    for (struct List* list_it = g_devices.next;
         list_it != &g_devices;
         list_it = list_it->next) {
        struct PciDevice* device = (struct PciDevice*)list_it;

        if (device->header->class_code == class_code &&
            device->header->subclass_code == subclass_code &&
            device->header->prog_if == prog_if) {
            returned = device;
            // return device;
        }
    }

    return returned;
}

physaddr_t
pci_device_get_memory_area(struct PciDevice* device, uint8_t bar_id, size_t* size) {
    volatile uint32_t* bars = NULL;
    size_t bar_count = 0;

    if (PCI_HEADER_TYPE_CHECK(device->header->header_type,
                              PCI_HEADER_TYPE_GENERAL)) {
        struct PciHeaderGeneral* ext_header = (void*)device->header;
        bars = (volatile uint32_t*) (uintptr_t) ext_header->bar;
        bar_count = 6;
    } else if (PCI_HEADER_TYPE_CHECK(device->header->header_type,
                                     PCI_HEADER_TYPE_PCI_TO_PCI)) {
        struct PciHeaderPciToPciBridge* ext_header = (void*)device->header;
        bars = (volatile uint32_t*) ext_header->bar;
        bar_count = 2;
    } else {
        panic("Unsupported PCI header type %02x", device->header->header_type);
        return 0;
    }

    if (bar_id >= bar_count) {
        return 0;
    }

    bool is_hibar = false;
    for (size_t i = 0; i < bar_id; ++i) {
        /* Skip hibar */
        if (is_hibar) {
            is_hibar = false;
            continue;
        }
        if (PCI_BAR_IS_MEMORY64(bars[i])) {
            is_hibar = true;
        }
    }

    /* Cannot map half-BAR */
    if (is_hibar) {
        return 0;
    }

    /* Cannot map IO bar */
    if (PCI_BAR_IS_IO(bars[bar_id])) {
        return 0;
    }

    if (PCI_BAR_IS_MEMORY64(bars[bar_id]))
    {
        assert(bar_id + 1 < bar_count);
        const uint32_t original_lo = bars[bar_id];
        const uint32_t original_hi = bars[bar_id + 1];

        bars[bar_id] |= PCI_BAR_MEMORY64_LOMASK;
        bars[bar_id + 1] |= PCI_BAR_MEMORY64_HIMASK;

        const uint64_t new_addr = PCI_BAR_MEMORY64_GET_ADDR(bars[bar_id],
                                                            bars[bar_id + 1]);
        bars[bar_id] = original_lo;
        bars[bar_id + 1] = original_hi;

        const uint64_t map_size = (~new_addr) + 1;
        const uint64_t map_addr = PCI_BAR_MEMORY64_GET_ADDR(bars[bar_id],
                                                            bars[bar_id + 1]);

        *size = map_size;
        return map_addr;
    }

    if (PCI_BAR_IS_MEMORY32(bars[bar_id]))
    {
        const uint32_t original = bars[bar_id];
        bars[bar_id] |= PCI_BAR_MEMORY32_MASK;

        const uint32_t new_addr = PCI_BAR_MEMORY32_GET_ADDR(bars[bar_id]);
        bars[bar_id] = original;

        const uint32_t map_size = (~new_addr) + 1;
        const uint32_t map_addr = PCI_BAR_MEMORY32_GET_ADDR(bars[bar_id]);

        *size = map_size;
        return map_addr;
    }

    panic("Unsupported PCI BAR type %03x", bars[bar_id] & 0x7);
    return 0;
}

uint32_t
pci_device_get_io_base(struct PciDevice* device, uint8_t bar_id) {
    volatile uint32_t* bars = NULL;
    size_t bar_count = 0;

    if (PCI_HEADER_TYPE_CHECK(device->header->header_type,
                              PCI_HEADER_TYPE_GENERAL)) {
        struct PciHeaderGeneral* ext_header = (void*)device->header;
        bars = (volatile uint32_t*) ext_header->bar;
        bar_count = 6;
    } else if (PCI_HEADER_TYPE_CHECK(device->header->header_type,
                                     PCI_HEADER_TYPE_PCI_TO_PCI)) {
        struct PciHeaderPciToPciBridge* ext_header = (void*)device->header;
        bars = (volatile uint32_t*) ext_header->bar;
        bar_count = 2;
    } else {
        panic("Unsupported PCI header type %02x", device->header->header_type);
        return 0;
    }

    if (bar_id >= bar_count) {
        return 0;
    }

    bool is_hibar = false;
    for (size_t i = 0; i < bar_id; ++i) {
        /* Skip hibar */
        if (is_hibar) {
            is_hibar = false;
            continue;
        }
        if (PCI_BAR_IS_MEMORY64(bars[i])) {
            is_hibar = true;
        }
    }

    /* Cannot map half-BAR */
    if (is_hibar) {
        return 0;
    }

    if (PCI_BAR_IS_IO(bars[bar_id])) {
        return PCI_BAR_IO_GET_ADDR(bars[bar_id]);
    }

    return 0;
}

void
dump_pci_tree(void) {
    cprintf("Found PCI buses:\n");
    for (struct List* list_it = g_buses.next;
         list_it != &g_buses;
         list_it = list_it->next) {
        struct PciBus* bus = (struct PciBus*)list_it;
        struct PciDevice* bridge = bus->as_device;

        cprintf("%02x from bridge %02x:%02x.%1o (class %02x.%02x.%02x)\n",
                (unsigned)bus->bus_id,
                (unsigned)(bridge->bus ? bridge->bus->bus_id : 0),
                (unsigned)PCI_DEVICE(bridge->devfn),
                (unsigned)PCI_FUNCTION(bridge->devfn),
                (unsigned)bridge->header->class_code,
                (unsigned)bridge->header->subclass_code,
                (unsigned)bridge->header->prog_if);
    }

    cprintf("Found PCI devices:\n");
    for (struct List* list_it = g_devices.next;
         list_it != &g_devices;
         list_it = list_it->next) {
        struct PciDevice* device = (struct PciDevice*)list_it;
        struct PciBus* bus = device->bus;

        cprintf("%02x:%02x.%1o of type %02x.%02x.%02x\n",
                (unsigned)(bus ? bus->bus_id : 0),
                (unsigned)PCI_DEVICE(device->devfn),
                (unsigned)PCI_FUNCTION(device->devfn),
                (unsigned)device->header->class_code,
                (unsigned)device->header->subclass_code,
                (unsigned)device->header->prog_if);
    }
}

inline static struct PciDevice* __attribute__((always_inline))
makeDevice(struct PciBus* bus, struct PciHeader* header, uint8_t devfn) {
    assert(g_deviceCount < MAX_PCI_DEVS);

    struct PciDevice* dev = &g_freeDevices[g_deviceCount++];
    dev->bus = bus;
    dev->devfn = devfn;
    dev->header = header;

    list_append(&g_devices, (struct List*)dev);

    return dev;
}

inline static struct PciBus* __attribute__((always_inline))
makeBus(struct PciDevice* bridge, uint8_t bus_num) {
    assert(g_busCount < MAX_PCI_BUSES);

    struct PciBus* bus = &g_freeBuses[g_busCount++];
    bus->as_device = bridge;
    bus->bus_id = bus_num;
    list_init(&bus->devices);
    list_init(&bus->buses);

    list_append(&g_buses, (struct List*)bus);

    return bus;
}


static void
scan_bus(struct PciDevice* bridge, uint64_t pci_base, uint8_t bus);

static void
scan_function(struct PciBus* bus, uint64_t pci_base, uint8_t dev, uint8_t fn) {
    const uint64_t paddr = PCI_DEV_PADDR(pci_base, bus->bus_id, dev, fn);
    struct PciHeader* header = mmio_map_region(paddr, PCI_ECAM_SIZE);
    if (header->vendor_id == PCI_VENDOR_INVALID) {
        return;
    }

    if (trace_pci) {
        cprintf("Found pci device %02x:%02x.%1o of type %02x.%02x.%02x\n",
                (unsigned)bus->bus_id, (unsigned)dev, (unsigned)fn,
                (unsigned)header->class_code,
                (unsigned)header->subclass_code,
                (unsigned)header->prog_if);
    }

    struct PciDevice* added = makeDevice(bus, header, PCI_DEVFN(dev, fn));
    list_append(&bus->devices, &added->child_list_node);

    if (PCI_HEADER_TYPE_CHECK(header->header_type, PCI_HEADER_TYPE_PCI_TO_PCI)) {
        struct PciHeaderPciToPciBridge* extended_header = (void*)header;
        scan_bus(added, pci_base, extended_header->secondary_bus_num);
    }
}

static void
scan_device(struct PciBus* bus, uint64_t pci_base, uint8_t dev) {
    const uint64_t paddr = PCI_DEV_PADDR(pci_base, bus->bus_id, dev, 0);
    struct PciHeader* header = mmio_map_region(paddr, PCI_ECAM_SIZE);

    if (header->vendor_id == PCI_VENDOR_INVALID) {
        return;
    }

    if (PCI_HEADER_TYPE_IS_MULTIFUNCTION(header->header_type)) {
        for (size_t i = 0; i < MAX_PCI_FN_PER_DEV; ++i) {
            scan_function(bus, pci_base, dev, i);
        }
    } else {
        scan_function(bus, pci_base, dev, 0);
    }
}

static void
scan_bus(struct PciDevice* bridge, uint64_t pci_base, uint8_t bus_num) {
    if (trace_pci) {
        cprintf("Found PCI bus %02x (bridge %02x:%02x.%1o, class %02x.%02x.%02x)\n",
                (unsigned)bus_num,
                (unsigned)(bridge->bus ? bridge->bus->bus_id : 0),
                (unsigned)PCI_DEVICE(bridge->devfn),
                (unsigned)PCI_FUNCTION(bridge->devfn),
                (unsigned)bridge->header->class_code,
                (unsigned)bridge->header->subclass_code,
                (unsigned)bridge->header->prog_if);
    }


    struct PciBus* bus = makeBus(bridge, bus_num);

    struct PciBus* parent_bus = bridge->bus;
    if (parent_bus != NULL) {
        list_append(&parent_bus->buses, &bus->child_list_node);
    } else {
        list_append(&g_pciRoots, &bus->child_list_node);
    }

    for (size_t i = 0; i < MAX_PCI_DEV_PER_BUS; ++i) {
        scan_device(bus, pci_base, i);
    }
}

void
pci_init(void) {
    MCFG* mcfg = get_mcfg();

    if (!mcfg) {
        panic("JOS requires PCIe!");
    }

    const size_t total_size = mcfg->header.Length;
    assert(mcfg->header.Length >= sizeof(*mcfg));

    const size_t content_size = total_size - sizeof(*mcfg);
    assert(content_size % sizeof(ECAMAddress) == 0);

    const size_t entry_count = content_size / sizeof(ECAMAddress);
    assert(entry_count > 0);
    if (entry_count > 1) {
        panic("JOS does not support multiple PCI segment groups! "
              "(found %zu groups total)",
              entry_count);
        return;
    }

    list_init(&g_buses);
    list_init(&g_devices);
    list_init(&g_pciRoots);

    const uint64_t pci_base = mcfg->groups[0].base_address;
    const uint64_t pci_root_paddr = PCI_DEV_PADDR(pci_base, 0, 0, 0);

    struct PciHeader* root_header = mmio_map_region(pci_root_paddr, PCI_ECAM_SIZE);
    if (trace_pci) {
        cprintf("Found Host-to-PCI bridge 00:00.0 (class %02x.%02x.%02x)\n",
                (unsigned)root_header->class_code,
                (unsigned)root_header->subclass_code,
                (unsigned)root_header->prog_if);
    }

    struct PciDevice* host_bridge = makeDevice(NULL, root_header, PCI_DEVFN(0, 0));

    scan_bus(host_bridge, pci_base, 0);

    if (PCI_HEADER_TYPE_IS_MULTIFUNCTION(root_header->header_type)) {
        for (size_t i = 1; i < MAX_PCI_FN_PER_DEV; ++i) {
            const uint64_t paddr = PCI_DEV_PADDR(pci_base, 0, 0, i);
            struct PciHeader* header = mmio_map_region(paddr, PCI_ECAM_SIZE);

            if (header->vendor_id == PCI_VENDOR_INVALID) {
                break;
            }
            if (trace_pci) {
                cprintf("Found Host-to-PCI bridge 00:00.%1o (class %02x.%02x.%02x)\n",
                        (unsigned)i,
                        (unsigned)header->class_code,
                        (unsigned)header->subclass_code,
                        (unsigned)header->prog_if);
            }

            host_bridge = makeDevice(NULL, header, PCI_DEVFN(0, i));
            scan_bus(host_bridge, pci_base, i);
        }
    }
    g_isPciInitialised = true;
}
