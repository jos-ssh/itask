#include "inc/kmod/acpi.h"
#include <inc/lib.h>
#include <inc/rpc.h>

static int acpid_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

static int acpid_serve_find_table(envid_t from, const void* request,
                                  void* response, int* response_perm);
                                
#define RECEIVE_ADDR 0x0FFFF000

static union AcpidResponse ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .HandlerCount = ACPID_NREQUESTS,
        .Handlers = {
                [ACPID_REQ_IDENTIFY] = acpid_serve_identify,
                // [ACPID_REQ_FIND_TABLE] = acpid_serve_find_table
        }};

void umain(int argc, char** argv) {
  cprintf("[%08x: acpid] Starting up module...\n", thisenv->env_id);
  rpc_serve(&Server); 
}

int
acpid_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    struct KmodIdentifyResponse* ident = (struct KmodIdentifyResponse*)response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = ACPID_VERSION;
    strncpy(ident->info.name, ACPID_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}
