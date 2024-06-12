#include "inc/convert.h"
#include "inc/env.h"
#include "inc/mmu.h"
#include "inc/stdio.h"
#include <inc/lib.h>

#include <inc/kmod/request.h>
#include <inc/kmod/init.h>
#include <inc/rpc.h>

#include "spawn.h"

// Return hex string of thisenv->env_id
static const char* envid_string(void);

// Serve IDENTIFY request
static int initd_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);
// Serve FIND_KMOD request
static int initd_serve_find_kmod(envid_t from, const void* request,
                                 void* response, int* response_perm);
// Serve FORK request
static int initd_serve_fork(envid_t from, const void* request, void* response,
                            int* response_perm);
// Serve SPAWN request
static int initd_serve_spawn(envid_t from, const void* request, void* response,
                             int* response_perm);

// Load module, passing hex string of initd's envid as argument
static int initd_load_module(const char* path);

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
        .Fallback = NULL,
        .HandlerCount = INITD_NREQUESTS,
        .Handlers = {
                [INITD_REQ_IDENTIFY] = initd_serve_identify,
                [INITD_REQ_FIND_KMOD] = initd_serve_find_kmod,
                [INITD_REQ_FORK] = initd_serve_fork,
                [INITD_REQ_SPAWN] = initd_serve_spawn}};

void
umain(int argc, char** argv) {
    cprintf("[%08x: initd] Starting up module...\n", thisenv->env_id);
    initd_load_module("/acpid");
    initd_load_module("/pcid");
    while (1) {
        rpc_listen(&Server, NULL);
    }
}

static int
initd_load_module(const char* path) {
    if (ModuleCount >= MAX_MODULES) {
        return -E_NO_MEM;
    }
    const char* argv[] = {path, envid_string(), NULL};
    int mod = initd_spawn(thisenv->env_id, path, argv);
    // int mod = spawnl(path, path, envid_string(), NULL);

    if (mod < 0) {
        cprintf("[%08x: initd] Failed to load module '%s': %i\n", thisenv->env_id, path, mod);
        return mod;
    }
    int res = initd_start_process(mod);
    if (res < 0) {
        cprintf("[%08x: initd] Failed to start module '%s': %i\n", thisenv->env_id, path, res);
        return res;
    }


    union KmodIdentifyResponse* response = (void*)RECEIVE_ADDR;
    res = rpc_execute(mod, KMOD_REQ_IDENTIFY, NULL, (void**)&response);
    if (res != 0 || !response) {
        cprintf("[%08x: initd] Bad module '%s'\n", thisenv->env_id, path);
        sys_env_destroy(mod);
        if (response) {
            sys_unmap_region(CURENVID, response, PAGE_SIZE);
        }

        return -E_INVAL;
    }

    cprintf("[%08x: initd] Loaded module '%s' v%zu from '%s' as env [%08x]\n",
            thisenv->env_id, response->info.name, response->info.version,
            path, mod);

    struct LoadedModule* module = &Modules[ModuleCount++];
    module->env = mod;
    module->info.version = response->info.version;
    memcpy(module->info.name, response->info.name, KMOD_MAXNAMELEN);
    sys_unmap_region(CURENVID, response, PAGE_SIZE);

    return 0;
}

int
initd_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
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

static int
initd_serve_fork(envid_t from, const void* request, void* response,
                 int* response_perm) {
    const volatile struct Env* parent = &envs[ENVX(from)];
    while (!parent->env_ipc_recving) {
      sys_yield();
    }
    assert(parent->env_status == ENV_NOT_RUNNABLE);
    int child = initd_fork(from);
    if (child < 0) {
      return child;
    }
    int res = initd_start_process(child);
    if (res < 0) {
      sys_env_destroy(child);
      return res;
    }

    // TODO: setup env_type and RUID
    return child;
}

static int
initd_serve_spawn(envid_t from, const void* request, void* response,
                  int* response_perm) {
    const union InitdRequest* spawnd_req = request;
    if (strnlen(spawnd_req->spawn.file, MAXPATHLEN) == MAXPATHLEN) {
        return -E_INVAL;
    }

    const char* argv[SPAWN_MAXARGS + 1] = {};

    if (spawnd_req->spawn.argc == 0) {
        argv[0] = spawnd_req->spawn.file;
        argv[1] = NULL;
    } else {
        if (spawnd_req->spawn.argc >= SPAWN_MAXARGS) { return -E_INVAL; }

        const size_t strtab_size = sizeof(spawnd_req->spawn.strtab);
        size_t last_end = 0;
        for (size_t i = 0; i < spawnd_req->spawn.argc; ++i) {
            const size_t start = spawnd_req->spawn.argv[i];
            const size_t maxlen = strtab_size - start;
            const size_t len = strnlen(spawnd_req->spawn.strtab + start, maxlen);

            if (len == maxlen) { return -E_INVAL; }
            if (start < last_end) { return -E_INVAL; }

            last_end = start + len + 1;
            argv[i] = spawnd_req->spawn.strtab + start;
        }

        argv[spawnd_req->spawn.argc] = NULL;
    }

    int child = initd_spawn(from, spawnd_req->spawn.file, argv);
    if (child < 0) return child;
    int res = initd_start_process(child);
    if (res < 0) {
        sys_env_destroy(child);
        return res;
    }
    // TODO: set env_type and ruid

    return child;
}

static const char*
envid_string(void) {
    static char str[32] = "";

    if (*str) return str;

    int cvt_res = ulong_to_str(thisenv->env_id, BASE_HEX, str, 32);
    assert(cvt_res > 0);

    return str;
}
