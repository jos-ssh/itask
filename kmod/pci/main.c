#include <inc/lib.h>
#include "inc/convert.h"
#include "inc/error.h"
#include "inc/kmod/pci.h"
#include <inc/rpc.h>

#include "inc/pci.h"
#include "inc/stdio.h"
#include "inc/types.h"
#include "pci.h"

#define RECEIVE_ADDR 0x0FFFF000

static union PcidResponse ResponseBuffer;

static int pcid_serve_identify(envid_t from, const void* request,
                               void* response, int* response_perm);

static int pcid_serve_lspci(envid_t from, const void* request,
                            void* response, int* response_perm);

static int pcid_serve_fallback(int32_t req_id, envid_t from, const void* request,
                               void* response, int* response_perm);

static int pcid_find_device(uint8_t class, uint8_t subclass, uint8_t prog,
                            union PcidResponse* response);

static int pcid_read_bar(uint8_t class, uint8_t subclass, uint8_t prog,
                         uint8_t bar, union PcidResponse* response);

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .Fallback = pcid_serve_fallback,
        .HandlerCount = PCID_NREQUESTS,
        .Handlers = {
                [PCID_REQ_IDENTIFY] = pcid_serve_identify,
                [PCID_REQ_LSPCI] = pcid_serve_lspci}};

void
umain(int argc, char** argv) {
    assert(argc > 1);
    unsigned long initd = 0;
    int cvt_res = str_to_ulong(argv[1], BASE_HEX, &initd);
    assert(cvt_res == 0);
    g_InitdEnvid = initd;

    cprintf("[%08x: pcid] Starting up module...\n", thisenv->env_id);
    while (1) {
        rpc_listen(&Server, NULL);
    }
}

int
pcid_serve_identify(envid_t from, const void* request,
                    void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = PCID_VERSION;
    strncpy(ident->info.name, PCID_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

int
pcid_serve_lspci(envid_t from, const void* request,
                 void* response, int* response_perm) {
    dump_pci_tree();
    return 0;
}

#define PCID_CLASS(id)    (((uint32_t)(id) & PCID_CLASS_MASK) >> 24)
#define PCID_SUBCLASS(id) (((uint32_t)(id) & PCID_SUBCLASS_MASK) >> 16)
#define PCID_PROG(id)     (((uint32_t)(id) & PCID_PROG_MASK) >> 8)
#define PCID_BAR(id)      (((uint32_t)(id) & PCID_REQ_MASK) >> 5)

int
pcid_serve_fallback(int32_t req_id, envid_t from, const void* request,
                    void* response, int* response_perm) {
#ifndef TEST_PCI
    enum EnvType type = envs[ENVX(from)].env_type;
    if (type != ENV_TYPE_FS && type != ENV_TYPE_KERNEL) {
        return -E_BAD_ENV;
    }
#endif // !TEST_PCI

    int res = -E_INVAL;

    switch (req_id & PCID_REQ_MASK) {
    case PCID_REQ_FIND_DEVICE:
        res = pcid_find_device(PCID_CLASS(req_id), PCID_SUBCLASS(req_id),
                               PCID_PROG(req_id), response);
        break;
    case PCID_REQ_READ_BAR:
        res = pcid_read_bar(PCID_CLASS(req_id), PCID_SUBCLASS(req_id),
                            PCID_PROG(req_id), PCID_BAR(req_id), response);
        break;
    default:
        return -E_INVAL;
    }
    if (res >= 0) {
        *response_perm = PROT_R;
    }

    return res;
}

static int
pcid_find_device(uint8_t class, uint8_t subclass, uint8_t prog,
                 union PcidResponse* response) {
    struct PciDevice* dev = find_device(class, subclass, prog);
    if (!dev) {
        return -E_NOT_FOUND;
    }

    struct PciHeader* header = dev->header;
    switch (header->header_type) {
    case PCI_HEADER_TYPE_GENERAL:
        memcpy(response, header, sizeof(struct PciHeaderGeneral));
        return sizeof(struct PciHeaderGeneral);
    case PCI_HEADER_TYPE_PCI_TO_PCI:
        memcpy(response, header, sizeof(struct PciHeaderPciToPciBridge));
        return sizeof(struct PciHeaderPciToPciBridge);
    default:
        return -E_NOT_SUPP;
    }
}

static int
pcid_read_bar(uint8_t class, uint8_t subclass, uint8_t prog,
              uint8_t bar, union PcidResponse* response) {
    struct PciDevice* dev = find_device(class, subclass, prog);
    if (!dev) {
        return -E_NOT_FOUND;
    }

    size_t size = 0;
    physaddr_t addr = pci_device_get_memory_area(dev, bar, &size);
    if (!addr) {
        return -E_NOT_SUPP;
    }

    response->bar.address = addr;
    response->bar.size = size;
    return 0;
}
