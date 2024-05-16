#include "inc/fs.h"
#include "inc/mmu.h"
#include <inc/lib.h>

#include <inc/kmod/request.h>
#include <inc/kmod/init.h>
#include <inc/rpc.h>

static int initd_serve_identify(envid_t from, const void* request,
                         void* response, int* response_perm);
static int initd_serve_find_kmod(envid_t from, const void* request,
                         void* response, int* response_perm);
static int initd_serve_read_acpi(envid_t from, const void* request,
                         void* response, int* response_perm);

#define RECEIVE_ADDR 0x0FFFF000

__attribute__((aligned(PAGE_SIZE)))
static uint8_t SendBuffer[PAGE_SIZE];

struct RpcServer Server = {
  .ReceiveBuffer = (void*) RECEIVE_ADDR,
  .SendBuffer = SendBuffer,
  .HandlerCount = INITD_NREQUESTS,
  .Handlers = {
    [INITD_REQ_IDENTIFY] = initd_serve_identify,
    // [INITD_REQ_READ_ACPI] = initd_serve_read_acpi,
    // [INITD_REQ_FIND_KMOD] = initd_serve_find_kmod
  }
};

void umain(int argc, char **argv) {
  rpc_serve(&Server);
}

int initd_serve_identify(envid_t from, const void* request,
                         void* response, int* response_perm) {
  struct KmodIdentifyResponse* ident = (struct KmodIdentifyResponse*) response;
  memset(ident, 0, sizeof(*ident));
  ident->info.version = INITD_VERSION;
  strncpy(ident->info.name, INITD_MODNAME, MAXNAMELEN);
  *response_perm = PROT_R;
  return 0;
}
