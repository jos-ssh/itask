/* hello, world */
#include "inc/env.h"
#include "inc/kmod/init.h"
#include <inc/assert.h>
#include <inc/kmod/pci.h>
#include "inc/kmod/net.h"
#include "inc/mmu.h"
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
find_netd(envid_t initd) {
    static union InitdRequest request;

    request.find_kmod.max_version = -1;
    request.find_kmod.min_version = -1;
    strcpy(request.find_kmod.name_prefix, NETD_MODNAME);

    void* res_data = NULL;
    return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, &res_data);
}

void check_teapot(envid_t netd) {
    // union NetdResponce* response = (void*) RECEIVE_ADDR;

    // static union NetdRequest req;
    // req.req = 42;

    // int res = rpc_execute(netd, NETD_IS_TEAPOT, &req,
    //     (void**)&response);

    // if (res < 0) {
    //   panic("teapot check failed: %i\n", res);
    // }

    // sys_unmap_region(CURENVID, response, PAGE_SIZE);
}

void
umain(int argc, char** argv) {
    envid_t initd = find_initd();

    envid_t netd = find_netd(initd);
    cprintf("Found 'netd' in env [%08x]\n", netd);

    cprintf("Currently, test is disabled. Nobody cared\n");
}
