#include <inc/rpc.h>
#include <inc/string.h>
#include <inc/lib.h>

#include "inc/kmod/request.h"
#include "inc/kmod/signal.h"
#include "signal.h"

volatile struct SigdSharedData g_SharedData[NENV];

union SigdRequest Request;
union KmodIdentifyResponse Response;

static int sigd_serve_identify(envid_t from, const void* request,
                               void* response, int* response_perm);

struct RpcServer Server = {
        .ReceiveBuffer = &Request,
        .SendBuffer = &Response,
        .Fallback = NULL,
        .HandlerCount = SIGD_NREQUESTS,
        .Handlers = {
                [SIGD_REQ_IDENTIFY] = sigd_serve_identify,

                [SIGD_NREQUESTS] = NULL}};

void
umain(int argc, char** argv) {
    cprintf("[%08x: sigd] Starting up module...\n", thisenv->env_id);
    for (;;) {
        rpc_listen(&Server, NULL);
    }
}

int
sigd_serve_identify(envid_t from, const void* request,
                    void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = SIGD_VERSION;
    strncpy(ident->info.name, SIGD_MODNAME, KMOD_MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}
