#include "inc/error.h"
#include "inc/fs.h"
#include "inc/kmod/request.h"
#include "inc/kmod/spawn.h"
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/stdio.h>
#include <inc/types.h>

#include "spawn.h"

#define RECEIVE_ADDR 0x0FFFF000

static union KmodIdentifyResponse ResponseBuffer;

static int spawnd_serve_identify(envid_t from, const void* request, void* response,
                             int* response_perm);
static int spawnd_serve_fork(envid_t from, const void* request, void* response,
                             int* response_perm);
static int spawnd_serve_spawn(envid_t from, const void* request, void* response,
                              int* response_perm);

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .Fallback = NULL,
        .HandlerCount = SPAWND_NREQUESTS,
        .Handlers = {
          [SPAWND_REQ_IDENTIFY] = spawnd_serve_identify,
          [SPAWND_REQ_FORK] = spawnd_serve_fork,
          [SPAWND_REQ_SPAWN] = spawnd_serve_spawn}};

void
umain(int argc, char** argv) {
    cprintf("[%08x: spawnd] Starting up module...\n", thisenv->env_id);
    while (1) {
        rpc_listen(&Server, NULL);
    }
}

static int spawnd_serve_identify(envid_t from, const void* request, void* response,
                             int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = SPAWND_VERSION;
    strncpy(ident->info.name, SPAWND_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static int spawnd_serve_fork(envid_t from, const void* request, void* response,
                             int* response_perm) {
  return spawnd_fork(from);
}

static int spawnd_serve_spawn(envid_t from, const void* request, void* response,
                              int* response_perm) {
  const union SpawndRequest* spawnd_req = request;
  if (strnlen(spawnd_req->env.file, MAXPATHLEN) == MAXPATHLEN) {
    return -E_INVAL;
  }

  if (spawnd_req->env.argc == 0) {
    const char* argv[] = { spawnd_req->env.file, NULL };
    return spawnd_spawn(from, spawnd_req->env.file, argv);
  }

  if (spawnd_req->env.argc >= SPAWND_MAXARGS) { return -E_INVAL; }
  const char* argv[SPAWND_MAXARGS + 1] = {};

  const size_t strtab_size = sizeof(spawnd_req->env.strtab);
  size_t last_end = 0;
  for (size_t i = 0; i < spawnd_req->env.argc; ++i)
  {
    const size_t start = spawnd_req->env.argv[i];
    const size_t maxlen = strtab_size - start;
    const size_t len = strnlen(spawnd_req->env.strtab + start, maxlen);

    if (len == maxlen) { return -E_INVAL; }
    if (start < last_end) { return -E_INVAL; }

    last_end = start + len + 1;
    argv[i] = spawnd_req->env.strtab + start;
  }

  argv[spawnd_req->env.argc] = NULL;

  return spawnd_spawn(from, spawnd_req->env.file, argv);
}
