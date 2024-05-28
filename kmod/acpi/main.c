#include "inc/acpi.h"
#include "inc/env.h"
#include "inc/error.h"
#include "inc/kmod/acpi.h"
#include "inc/mmu.h"
#include "kmod/acpi/acpi.h"
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
                [ACPID_REQ_FIND_TABLE] = acpid_serve_find_table
        }};

void umain(int argc, char** argv) {
  // TODO: Test module

  cprintf("[%08x: acpid] Starting up module...\n", thisenv->env_id);
  while (1) {
    rpc_listen(&Server, NULL);
  }
}

int
acpid_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = ACPID_VERSION;
    strncpy(ident->info.name, ACPID_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static int acpid_serve_find_table(envid_t from, const void* request,
                                  void* response, int* response_perm) {
  enum EnvType type = envs[ENVX(from)].env_type;
  if (type != ENV_TYPE_FS && type != ENV_TYPE_KERNEL) {
    return -E_BAD_ENV;
  }
  const union AcpidRequest* acpid_req = request;

  // TODO: Make all functions return const pointers
  const ACPISDTHeader* header = acpi_find_table(acpid_req->find_table.Signature);
  if (!header) {
    return -E_INVAL;
  }
  
  if (header->Length <= acpid_req->find_table.Offset) {
    return -E_INVAL;
  }

  union AcpidResponse* acpid_res = response;
  memset(acpid_res, 0, sizeof(*acpid_res));
  
  size_t copy_size = header->Length - acpid_req->find_table.Offset;
  if (copy_size > PAGE_SIZE) {
    copy_size = PAGE_SIZE;
  }

  memcpy(acpid_res, ((const char*)header) + acpid_req->find_table.Offset, copy_size);
  *response_perm = PROT_R;

  return copy_size;
}
