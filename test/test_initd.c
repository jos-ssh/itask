#include "inc/env.h"
#include "inc/kmod/init.h"
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
                return envs[i].env_id;
            }
        }
    return 0;
}

void
test_invalid_request(envid_t initd) {
  void* res_data = NULL;
  int res = rpc_execute(initd, INITD_NREQUESTS + 42, NULL, &res_data);
  assert(res == -E_INVAL);
  assert(res_data == NULL);
}

void
umain(int argc, char** argv) {
    envid_t initd = find_initd();
    cprintf("Found 'initd' in env [%08x]\n", initd);

    test_invalid_request(initd);
    test_invalid_request(initd);
}
