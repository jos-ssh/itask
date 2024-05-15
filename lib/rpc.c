#include "inc/env.h"
#include "inc/error.h"
#include "inc/mmu.h"
#include <inc/rpc.h>
#include <inc/lib.h>

void
rpc_serve(const struct RpcServer* server) {
    while (1) {
        envid_t from = 0;
        int perm = 0;
        size_t size = PAGE_SIZE;
        uint32_t req_id = ipc_recv(&from, server->ReceiveBuffer, &size, &perm);

        int response_perm = 0;
        void* request = (perm & PROT_R) ? server->ReceiveBuffer : NULL;
        int result = 0;
        if (req_id >= server->HanlderCount || server->Handlers[req_id] == NULL) {
            cprintf("[%08x]: Invalid RPC request from [%08x], invalid id %u\n",
                    sys_getenvid(), from, req_id);
            result = -E_INVAL;
        } else {
            result = server->Handlers[req_id](from, request,
                                              server->SendBuffer, &response_perm);
        }

        if (response_perm & PROT_R) {
            ipc_send(from, result, server->SendBuffer, PAGE_SIZE, response_perm);
        } else {
            ipc_send(from, result, NULL, 0, 0);
        }
        sys_unmap_region(CURENVID, server->ReceiveBuffer, PAGE_SIZE);
    }
}
