#include "inc/env.h"
#include "inc/error.h"
#include "inc/kmod/net.h"
#include "inc/mmu.h"
#include <inc/lib.h>
#include <inc/rpc.h>

static int netd_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

static int netd_serve_teapot(envid_t from, const void* request,
                                    void* response, int* response_perm);
                                
#define RECEIVE_ADDR 0x0FFFF000

static union NetdResponce ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .HandlerCount = NETD_NREQUESTS,
        .Handlers = {
                [IDENTITY] = netd_serve_identify,
                [IS_TEAPOT] = netd_serve_teapot
        }};

void umain(int argc, char** argv) {
  cprintf("[%08x: virtionetd] Starting up module...\n", thisenv->env_id);
  while (1) {
    rpc_listen(&Server, NULL);
  }
}

static int
netd_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = NETD_VERSION;
    strncpy(ident->info.name, NETD_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static int netd_serve_teapot(envid_t from, const void* request,
                                  void* response, int* response_perm) {
  const union NetdRequest* req = request;

  cprintf("[%08x: virtionetd] Requested teapot with code %d", thisenv->env_id, (int)req->req);

  union NetdResponce* res = response;
  memset(res, 0, sizeof(*res));
  res->res = req->req;

  return 1;
}
