#include <inc/lib.h>
#include "inc/convert.h"
#include "inc/error.h"
#include "inc/kmod/pci.h"
#include <inc/rpc.h>

#include "pci.h"

#define RECEIVE_ADDR 0x0FFFF000

static union PcidResponse ResponseBuffer;

static int pcid_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

static int pcid_serve_lspci(envid_t from, const void* request,
                                  void* response, int* response_perm);

static int pcid_serve_fallback(int32_t req_id, envid_t from, const void* request,
                                  void* response, int* response_perm);

static int pcid_find_device(uint8_t class, uint8_t subclass, uint8_t prog,
                            union PcidResponse* response);
static int pcid_map_region(uint8_t class, uint8_t subclass, uint8_t prog,
                           envid_t from, const struct PcidMapRegion* request);

struct RpcServer Server = {
    .ReceiveBuffer = (void*)RECEIVE_ADDR,
    .SendBuffer = &ResponseBuffer,
    .Fallback = pcid_serve_fallback,
    .HandlerCount = PCID_NREQUESTS,
    .Handlers = {
      [PCID_REQ_IDENTIFY] = pcid_serve_identify,
      [PCID_REQ_LSPCI] = pcid_serve_lspci
    }
};

void umain(int argc, char** argv) {
  assert(argc > 1);
  unsigned long initd = 0;
  int cvt_res = str_to_ulong(argv[1], BASE_HEX, &initd);
  assert(cvt_res == 0);
  g_InitdEnvid = initd;

  cprintf("[%08x: pcid] Starting up module...\n", thisenv->env_id);
  while (1) {
    rpc_listen(&Server, NULL);
  }
}

int
pcid_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = PCID_VERSION;
    strncpy(ident->info.name, PCID_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

int pcid_serve_lspci(envid_t from, const void* request,
                                  void* response, int* response_perm) {
    dump_pci_tree();
    return 0;
}

int pcid_serve_fallback(int32_t req_id, envid_t from, const void* request,
                                  void* response, int* response_perm) {
#ifndef TEST_PCI
  enum EnvType type = envs[ENVX(from)].env_type;
  if (type != ENV_TYPE_FS && type != ENV_TYPE_KERNEL) {
    return -E_BAD_ENV;
  }
#endif // !TEST_PCI

  // TODO: implement
  return -E_INVAL;
}
