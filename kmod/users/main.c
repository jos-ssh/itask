#include <inc/kmod/users.h>
#include <inc/rpc.h>

#include <inc/lib.h>

static int usersd_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);


#define RECEIVE_ADDR 0x0FFFF000
static union UsersdResponse ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .Fallback = NULL,
        .HandlerCount = USERSD_NREQUESTS,
        .Handlers = {
                [USERSD_REQ_IDENTIFY] = usersd_serve_identify}
};

void umain(int argc, char** argv) {
  printf("[%08x: usersd] Starting up module...\n", thisenv->env_id);
  while (1) {
    rpc_listen(&Server, NULL);
  }
}

int
usersd_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = USERSD_VERSION;
    strncpy(ident->info.name, USERSD_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}