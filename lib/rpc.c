#include "inc/env.h"
#include "inc/error.h"
#include "inc/mmu.h"
#include <inc/rpc.h>
#include <inc/lib.h>

int
rpc_listen(const struct RpcServer* server, struct RpcFailure* failure) {
    bool handled = false;
    while (!handled) {
        envid_t from = 0;
        int perm = 0;
        size_t size = PAGE_SIZE;
        uint32_t req_id = ipc_recv(&from, server->ReceiveBuffer, &size, &perm);

        int response_perm = 0;
        void* request = (perm & PROT_R) ? server->ReceiveBuffer : NULL;
        int result = 0;
        if (req_id >= server->HandlerCount || server->Handlers[req_id] == NULL) {
            if (failure) {
              failure->Source = from;
              failure->RequestId = req_id;
              failure->Request = request;
              failure->Perm = perm;

              return -1;
            }
            cprintf("[%08x]: Invalid RPC request from [%08x], invalid id %u\n",
                    sys_getenvid(), from, req_id);
            result = -E_INVAL;
        } else {
            result = server->Handlers[req_id](from, request,
                                              server->SendBuffer, &response_perm);
            handled = true;
        }

        if (response_perm & PROT_R) {
            ipc_send(from, result, server->SendBuffer, PAGE_SIZE, response_perm);
        } else {
            ipc_send(from, result, NULL, 0, 0);
        }
        sys_unmap_region(CURENVID, server->ReceiveBuffer, PAGE_SIZE);
    }
    return 0;
}

int32_t rpc_execute(envid_t server, int32_t req_id, const void* req_data, void** res_data) {
  assert(res_data);
  if (req_data) {
    ipc_send(server, req_id, (void*) req_data, PAGE_SIZE, PROT_R);
  } else {
    ipc_send(server, req_id, NULL, 0, 0);
  }
  
  int32_t res = 0;
  if (res_data) {
    int recv_perm = 0;
    size_t max_size = PAGE_SIZE;
    res = ipc_recv_from(server, *res_data, &max_size, &recv_perm);

    if (!(recv_perm & PROT_R)) {
      // No response data
      *res_data = NULL;
    }
  } else {
    res = ipc_recv_from(server, NULL, NULL, 0);
  }

  return res;
}
