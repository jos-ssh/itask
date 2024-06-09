#include "inc/kmod/spawn.h"
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/stdio.h>
#include <inc/types.h>

#define RECEIVE_ADDR 0x0FFFF000

static int spawnd_serve_fork(envid_t from, const void* request, void* response,
                             int* response_perm);
static int spawnd_serve_spawn(envid_t from, const void* request, void* response,
                              int* response_perm);
static int spawnd_serve_exec(envid_t from, const void* request, void* response,
                             int* response_perm);

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = NULL,
        .Fallback = NULL,
        .HandlerCount = SPAWND_NREQUESTS,
        .Handlers = {}};

void
umain(int argc, char** argv) {
    cprintf("[%08x: spawnd] Starting up module...\n", thisenv->env_id);
    while (1) {
        rpc_listen(&Server, NULL);
    }
}
