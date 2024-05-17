#include "inc/env.h"
#include "inc/mmu.h"
#include <inc/lib.h>

#include <inc/kmod/request.h>
#include <inc/kmod/init.h>
#include <inc/rpc.h>

static int initd_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);
static int initd_serve_find_kmod(envid_t from, const void* request,
                                 void* response, int* response_perm);

#define MAX_MODULES  256
#define RECEIVE_ADDR 0x0FFFF000

struct LoadedModule {
    envid_t env;
    struct KmodInfo info;
};

static size_t ModuleCount = 0;
static struct LoadedModule Modules[MAX_MODULES];

__attribute__((aligned(PAGE_SIZE))) static uint8_t SendBuffer[PAGE_SIZE];

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = SendBuffer,
        .HandlerCount = INITD_NREQUESTS,
        .Handlers = {
                [INITD_REQ_IDENTIFY] = initd_serve_identify,
                [INITD_REQ_FIND_KMOD] = initd_serve_find_kmod
        }};

void
umain(int argc, char** argv) {
    rpc_serve(&Server);
}

int
initd_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    struct KmodIdentifyResponse* ident = (struct KmodIdentifyResponse*)response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = INITD_VERSION;
    strncpy(ident->info.name, INITD_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static int
initd_serve_find_kmod(envid_t from, const void* request,
                      void* response, int* response_perm) {
    const struct InitdFindKmod* req = request;
    const size_t prefix_len = strnlen(req->name_prefix, KMOD_MAXNAMELEN);
    for (size_t i = 0; i < ModuleCount; ++i) {
        if (strncmp(req->name_prefix, Modules[i].info.name, prefix_len) != 0) {
            continue;
        }
        if (req->min_version >= 0 && Modules[i].info.version < req->min_version) {
            continue;
        }
        if (req->max_version >= 0 && Modules[i].info.version > req->max_version) {
            continue;
        }
        return Modules[i].env;
    }
    return 0;
}
