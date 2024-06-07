/* hello, world */
#include "inc/env.h"
#include "inc/kmod/init.h"
#include <inc/assert.h>
#include <inc/kmod/pci.h>
#include "inc/mmu.h"
#include "inc/pci.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

#define RECEIVE_ADDR 0x0FFFF000

envid_t
find_initd() {
    for (size_t i = 0; i < NENV; i++)
        if (envs[i].env_type == ENV_TYPE_KERNEL) {
            union KmodIdentifyResponse* response = (void*)RECEIVE_ADDR;

            int res = rpc_execute(envs[i].env_id, KMOD_REQ_IDENTIFY, NULL, (void**)&response);
            assert(res == 0);

            int namelen = strnlen(response->info.name, KMOD_MAXNAMELEN);

            cprintf("Kernel type env [%08x] is module '%*s' v%zu\n",
                    envs[i].env_id, namelen, response->info.name,
                    response->info.version);

            if (strcmp(INITD_MODNAME, response->info.name) == 0) {
                sys_unmap_region(CURENVID, response, PAGE_SIZE);
                return envs[i].env_id;
            }
            sys_unmap_region(CURENVID, response, PAGE_SIZE);
        }
    return 0;
}


envid_t
find_pcid(envid_t initd) {
    static union InitdRequest request;

    request.find_kmod.max_version = -1;
    request.find_kmod.min_version = -1;
    strcpy(request.find_kmod.name_prefix, PCID_MODNAME);

    void* res_data = NULL;
    return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, &res_data);
}

void lspci(envid_t pcid) {
    void* resp = NULL;
    rpc_execute(pcid, PCID_REQ_LSPCI, NULL, &resp);
}

void check_nvme(envid_t pcid) {
    union PcidResponse* response = (void*) RECEIVE_ADDR;

    int res = rpc_execute(pcid, PCID_REQ_FIND_DEVICE | pcid_device_id(1, 8, 2), NULL,
        (void**)&response);

    if (res < 0) {
      panic("NVME check failed: %i\n", res);
    }
    assert(res >= sizeof(struct PciHeader));
    assert(res == sizeof(struct PciHeaderGeneral));
    assert(response->device.class_code == 1);
    assert(response->device.subclass_code == 8);
    assert(response->device.prog_if == 2);

    sys_unmap_region(CURENVID, response, PAGE_SIZE);
}

void
umain(int argc, char** argv) {
    envid_t initd = find_initd();

    envid_t pcid = find_pcid(initd);
    cprintf("Found 'pcid' in env [%08x]\n", pcid);

    lspci(pcid);
    check_nvme(pcid);
}
