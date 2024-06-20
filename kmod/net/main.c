#include <inc/error.h>
#include <inc/kmod/net.h>
#include <inc/kmod/init.h>
#include <inc/kmod/pci.h>
#include <inc/mmu.h>
#include <inc/convert.h>
#include <inc/lib.h>
#include <inc/rpc.h>

#include "inc/stdio.h"
#include "net.h"

static int netd_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

static int netd_serve_teapot(envid_t from, const void* request,
                                    void* response, int* response_perm);

envid_t g_InitdEnvid;
envid_t g_PcidEnvid;
struct virtio_net_device_t net;

static union NetdResponce ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .HandlerCount = NETD_NREQUESTS,
        .Handlers = {
                [NETD_IDENTITY] = netd_serve_identify,
        }};

void umain(int argc, char** argv) {
  assert(argc > 1);
  unsigned long initd = 0;
  int cvt_res = str_to_ulong(argv[1], BASE_HEX, &initd);
  assert(cvt_res == 0);
  g_InitdEnvid = initd;

  cprintf("[%08x: netd] Starting up module...\n", thisenv->env_id);

  // Answer initd
  rpc_listen(&Server, NULL);

  initialize();

  while (1) {
    uint8_t status = *net.isr_status;
    if (status & VIRTIO_PCI_ISR_NOTIFY) {
        process_queue(&net.recvq, true);
        process_queue(&net.sendq, false);
    }

    sys_yield();
  }
}

/* #### Simple oneshot replies ####
*/

static int
netd_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = NETD_VERSION;
    strncpy(ident->info.name, NETD_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}
