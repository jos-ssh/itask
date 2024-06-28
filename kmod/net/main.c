#include <inc/error.h>
#include <inc/kmod/net.h>
#include <inc/kmod/init.h>
#include <inc/kmod/pci.h>
#include <inc/mmu.h>
#include <inc/convert.h>
#include <inc/lib.h>
#include <inc/rpc.h>
#include <stdatomic.h>

#include "inc/stdio.h"
#include "net.h"

#include "connection.h"

static int netd_serve_identify(envid_t from, const void* request,
                               void* response, int* response_perm);

static int netd_serve_recieve(envid_t from, const void* request,
                              void* response, int* response_perm);

static int netd_serve_send(envid_t from, const void* request,
                           void* response, int* response_perm);

static int netd_serve_poll(envid_t from, const void* request,
                           void* response, int* response_perm);

envid_t g_InitdEnvid;
envid_t g_PcidEnvid;

static int netd_start_loop();

struct Connection g_Connection = {{0, 0}, kCreated};
struct RingBuffer __attribute__((aligned(PAGE_SIZE))) g_SendBuffer = {0, 0};

bool StartupDone = false;

struct virtio_net_device_t* net = (void*)UTEMP;
static union NetdResponce ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .HandlerCount = NETD_NREQUESTS,
        .Handlers = {
                [NETD_IDENTITY] = netd_serve_identify,
                [NETD_REQ_RECIEVE] = netd_serve_recieve,
                [NETD_REQ_SEND] = netd_serve_send,
                [NETD_REQ_POLL] = netd_serve_poll,
        }};

void
umain(int argc, char** argv) {
    assert(argc > 1);
    unsigned long initd = 0;
    int cvt_res = str_to_ulong(argv[1], BASE_HEX, &initd);
    assert(cvt_res == 0);
    g_InitdEnvid = initd;

    cprintf("[%08x: netd] Starting up module...\n", thisenv->env_id);

    for (;;) {
        bool already_done = StartupDone;
        rpc_listen(&Server, NULL);
        if (!already_done && StartupDone) {
            cprintf("netd start loop\n");
            netd_start_loop();
        }
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

    StartupDone = true;
    cprintf("netd serve identify\n");
    return 0;
}

static int
netd_serve_recieve(envid_t from, const void* request,
                   void* response, int* response_perm) {
    if (atomic_load(&g_Connection.state) == kFinished) {
        return -E_CONNECTION_СLOSED;
    }
    const struct NetdRecieve* req = request;
    if (req->target != from) {
        return -E_BAD_ENV;
    }

    struct NetdRecieveData* res = response;
    res->size = read_buf(&g_Connection.recieve_buf, res->data, req->size);
    *response_perm = PROT_R;
    return res->size;
}


static int
netd_serve_send(envid_t from, const void* request,
                void* response, int* response_perm) {
    if (atomic_load(&g_Connection.state) == kFinished) {
        return -E_CONNECTION_СLOSED;
    }
    const struct NetdSend* req = request;
    struct Message msg = {req->size};
    memcpy(msg.data, req->data, req->size);
    write_buf(&g_SendBuffer, (const unsigned char*)&msg, msg.size + sizeof(msg.size));
    return 0;
}


static int
netd_serve_poll(envid_t from, const void* request,
                void* response, int* response_perm) {
    const struct NetdPoll* req = request;
    assert(req->target == from);
    if (atomic_load(&g_Connection.state) == kFinished) {
        return -E_CONNECTION_СLOSED;
    }

    return size_buf(&g_Connection.recieve_buf);
}

static int
netd_start_loop() {
    static union InitdRequest initd_req;
    void* res_data = NULL;

    initd_req.fork.parent = CURENVID;
    envid_t parent = thisenv->env_id;
    rpc_execute(kmod_find_any_version(INITD_MODNAME), INITD_REQ_FORK, &initd_req, &res_data);
    thisenv = &envs[ENVX(sys_getenvid())];

    int res = thisenv->env_ipc_value;
    if (res < 0) return res;

    if (res == 0) {
        netd_process_loop(parent);
        panic("Unreachable code");
    }

    envid_t child = res;

    static_assert(sizeof(g_Connection) % PAGE_SIZE == 0, "Unaligned shared data");
    res = sys_map_region(CURENVID, &g_Connection, child, &g_Connection,
                         sizeof(g_Connection), PROT_RW | PROT_SHARE);
    if (res < 0) goto error;

    res = sys_map_region(CURENVID, &g_SendBuffer, child, &g_SendBuffer,
                         sizeof(g_SendBuffer), PROT_RW | PROT_SHARE);
    if (res < 0) goto error;

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) goto error;

    return child;
error:
    sys_env_destroy(child);
    return res;
}