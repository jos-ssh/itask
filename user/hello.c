/* hello, world */
#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>
#include <inc/acpi.h>
#include <inc/kmod/acpi.h>

#define RECEIVE_ADDR 0x0FFFF000


envid_t
find_initd() {
    for (size_t i = 0; i < NENV; i++)
        if (envs[i].env_type == ENV_TYPE_KERNEL) {
            struct KmodIdentifyResponse* response = (void*)RECEIVE_ADDR;

            int res = rpc_execute(envs[i].env_id, KMOD_REQ_IDENTIFY, NULL, (void**)&response);
            assert(res == 0);

            int namelen = strnlen(response->info.name, KMOD_MAXNAMELEN);

            cprintf("Kernel type env [%08x] is module '%*s' v%zu\n",
                    envs[i].env_id, namelen, response->info.name,
                    response->info.version);

            if (strcmp(INITD_MODNAME, response->info.name) == 0) {
                return envs[i].env_id;
            }
        }
    return 0;
}


envid_t
find_acpid(envid_t initd) {
  __attribute__((aligned(PAGE_SIZE)))
  static union InitdRequest request;

  request.find_kmod.max_version = -1;
  request.find_kmod.min_version = -1;
  strcpy(request.find_kmod.name_prefix, "jos.core.acpi");

  return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, NULL);
}

typedef struct {
    uint64_t BaseAddress;
    uint16_t SegmentGroup;
    uint8_t StartBus;
    uint8_t EndBus;
    uint32_t Reserved;
} CSBAA;

typedef struct {
    ACPISDTHeader h;
    uint64_t Reserved;
    CSBAA Data[];
} MCFG;

MCFG* get_mcfg(envid_t acpid) {
  static union AcpidRequest req;

  strncpy(req.find_table.Signature, "MCFG", 4);
  req.find_table.Offset = 0;

  union AcpidResponse* res = (void*) RECEIVE_ADDR;
  int status = rpc_execute(acpid, ACPID_REQ_FIND_TABLE, &req, (void**)&res);
  if (status < 0) {
    panic("Failed to read MCFG: %i\n", status);
  }
  assert(status == res->table_start.Header.Length);

  return (MCFG*)(res);
}

void
umain(int argc, char** argv) {
    cprintf("hello, world\n");
    cprintf("i am environment %08x\n", thisenv->env_id);

    envid_t initd = find_initd();
    cprintf("Found 'initd' in env [%08x]\n", initd);

    envid_t acpid = find_acpid(initd);
    cprintf("Found 'acpid' in env [%08x]\n", acpid);

    MCFG* mcfg = get_mcfg(acpid);
    cprintf("MCFG checksum=%04x\n", (uint32_t) mcfg->h.Checksum);
}
