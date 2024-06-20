#include "inc/assert.h"
#include "inc/error.h"
#include "inc/stdio.h"
#include <inc/env.h>
#include <inc/kmod/request.h>
#include <inc/kmod/init.h>
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/string.h>

static union KmodIdentifyResponse Response;

static union KmodIdentifyResponse* const ResponseAddr = &Response;

static envid_t
get_initd() {
    static envid_t initd = 0;

    if (initd) { return initd; }

    int res = sys_unmap_region(CURENVID, &Response, sizeof(Response));
    if (res < 0) { panic("sys_unmap_region: %i\n", res); }

    for (size_t i = 0; i < NENV; i++) {
        if (envs[i].env_type == ENV_TYPE_KERNEL) {
            union KmodIdentifyResponse* response = ResponseAddr;

            res = rpc_execute(envs[i].env_id, KMOD_REQ_IDENTIFY, NULL, (void**)&response);
            assert(res == 0);

#if 0
            int namelen = strnlen(response->info.name, KMOD_MAXNAMELEN);
            cprintf("Kernel type env [%08x] is module '%*s' v%zu\n",
                    envs[i].env_id, namelen, response->info.name,
                    response->info.version);
#endif

            if (strcmp(INITD_MODNAME, response->info.name) == 0) {
                initd = envs[i].env_id;
                return initd;
            }
        }
    }

    panic("Failed to find initd!");
    return 0;
}

int
kmod_find(const char* name_prefix, int min_version, int max_version) {
    static union InitdRequest request;

    if (strcmp(INITD_MODNAME, name_prefix) == 0) {
        if (min_version >= 0 && min_version > INITD_VERSION) { return -E_NOT_FOUND; }
        if (max_version >= 0 && max_version < INITD_VERSION) { return -E_NOT_FOUND; }

        return get_initd();
    }

    request.find_kmod.max_version = min_version;
    request.find_kmod.min_version = max_version;
    strncpy(request.find_kmod.name_prefix, name_prefix, KMOD_MAXNAMELEN);

    void* res_data = NULL;
    return rpc_execute(get_initd(), INITD_REQ_FIND_KMOD, &request, &res_data);
}

int
kmod_find_any_version(const char* name_prefix) {
    return kmod_find(name_prefix, -1, -1);
}
