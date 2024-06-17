#include <inc/kmod/request.h>
#include <inc/kmod/file.h>
#include <inc/rpc.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/lib.h>

static union FiledRequest Request;
static union FiledResponse Response;

static int filed_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

struct RpcServer Server = {
  .ReceiveBuffer = &Request,
  .SendBuffer = &Response,
  .Fallback = NULL,
  .HandlerCount = FILED_NREQUESTS,
  .Handlers = {
    [FILED_REQ_IDENTIFY] = filed_serve_identify,

    [FILED_NREQUESTS] = NULL
  }
};

void umain(int argc, char** argv) {
    cprintf("[%08x: filed] Starting up module...\n", thisenv->env_id);
    while (1) {
      rpc_listen(&Server, NULL);
    }
}

int
filed_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = FILED_VERSION;
    strncpy(ident->info.name, FILED_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}
