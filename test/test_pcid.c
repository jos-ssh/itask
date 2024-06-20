/* hello, world */
#include "inc/env.h"
#include <inc/assert.h>
#include <inc/kmod/pci.h>
#include "inc/mmu.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

#define RECEIVE_ADDR 0x0FFFF000

void lspci(envid_t pcid) {
    cprintf("Listing pci devices...\n");
    void* resp = NULL;
    rpc_execute(pcid, PCID_REQ_LSPCI, NULL, &resp);
}

void check_nvme(envid_t pcid) {
    cprintf("Checking NVME pci device...\n");
    union PcidResponse* response = (void*) RECEIVE_ADDR;

    int res = rpc_execute(pcid, PCID_REQ_READ_BAR | pcid_bar_id(1, 8, 2, 0), NULL,
        (void**)&response);

    if (res < 0) {
      panic("NVME check failed: %i\n", res);
    }
    cprintf("NVME BAR#0: 0x%016lx (0x%zx bytes)\n",
        response->bar.address, response->bar.size);

    sys_unmap_region(CURENVID, response, PAGE_SIZE);
}

void
umain(int argc, char** argv) {
    envid_t pcid = kmod_find_any_version(PCID_MODNAME);
    cprintf("Found 'pcid' in env [%08x]\n", pcid);

    lspci(pcid);
    check_nvme(pcid);
}
