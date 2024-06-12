#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

#define RECEIVE_ADDR 0x0FFFF000

envid_t
find_initd() {
    for (size_t i = 0; i < NENV; i++)
        if (envs[i].env_type == ENV_TYPE_KERNEL) {
            union KmodIdentifyResponse* response = (void*)RECEIVE_ADDR;

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

/*
void
test_invalid_request(envid_t initd) {
  void* res_data = NULL;
  int res = rpc_execute(initd, INITD_NREQUESTS + 42, NULL, &res_data);
  assert(res == -E_INVAL);
  assert(res_data == NULL);
}
*/

static union InitdRequest request;

void
check_spawn_date(envid_t initd) {
  strcpy(request.spawn.file, "/date");
  request.spawn.argc = 0;

  printf("INITD CHECK: spawn date\n");
  void* res_data = NULL;
  int res = rpc_execute(initd, INITD_REQ_SPAWN, &request, &res_data);

  if (res < 0) {
    panic("spawn error: %i", res);
  }

  printf("INITD LOG: spawned 'date' as [%08x]\n", res);
  wait(res);
};

void
check_spawn_echo(envid_t initd) {
  strcpy(request.spawn.file, "/echo");
  request.spawn.argc = 3;
  char arg0[] = "echo";
  char arg1[] = "Hello, ";
  char arg2[] = "world!";
  
  request.spawn.argv[0] = 0;
  request.spawn.argv[1] = sizeof(arg0);
  request.spawn.argv[2] = sizeof(arg0) + sizeof(arg1);

  memcpy(request.spawn.strtab, arg0, sizeof(arg0));
  memcpy(request.spawn.strtab + sizeof(arg0), arg1, sizeof(arg1));
  memcpy(request.spawn.strtab + sizeof(arg0) + sizeof(arg1), arg2, sizeof(arg2));

  printf("INITD CHECK: spawn echo\n");
  void* res_data = NULL;
  int res = rpc_execute(initd, INITD_REQ_SPAWN, &request, &res_data);

  if (res < 0) {
    panic("spawn error: %i", res);
  }

  printf("INITD LOG: spawned 'echo' as [%08x]\n", res);
  wait(res);
}

void
check_fork(envid_t initd) {
  void* res_data = NULL;
  printf("INITD CHECK: fork\n");
  int res = rpc_execute(initd, INITD_REQ_FORK, NULL, &res_data);
  if (res < 0) {
    panic("spawn error: %i", res);
  }

  if (res == 0) {
    printf("INITD LOG: child running\n");
    return;
  }

  printf("INITD LOG: forked child [%08x]\n", res);
}

void
umain(int argc, char** argv) {
    envid_t initd = find_initd();
    cprintf("Found 'initd' in env [%08x]\n", initd);

    /*
    test_invalid_request(initd);
    test_invalid_request(initd);
    */
    check_spawn_date(initd);
    check_spawn_echo(initd);
    check_fork(initd);
}
