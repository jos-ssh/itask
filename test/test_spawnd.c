/* hello, world */
#include "inc/env.h"
#include "inc/kmod/init.h"
#include <inc/assert.h>
#include <inc/kmod/spawn.h>
#include "inc/mmu.h"
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

            printf("Kernel type env [%08x] is module '%*s' v%zu\n",
                    envs[i].env_id, namelen, response->info.name,
                    response->info.version);

            if (strcmp(INITD_MODNAME, response->info.name) == 0) {
                sys_unmap_region(CURENVID, response, PAGE_SIZE);
                return envs[i].env_id;
            }
            sys_unmap_region(CURENVID, response, PAGE_SIZE);
        }
    return 0;
}

envid_t
find_spawnd(envid_t initd) {
    static union InitdRequest request;

    request.find_kmod.max_version = -1;
    request.find_kmod.min_version = -1;
    strcpy(request.find_kmod.name_prefix, SPAWND_MODNAME);

    void* res_data = NULL;
    return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, &res_data);
}

static union SpawndRequest request;

void
check_spawn_date(envid_t spawnd) {
  strcpy(request.env.file, "/date");
  request.env.argc = 0;

  printf("SPAWND CHECK: spawn date\n");
  void* res_data = NULL;
  int res = rpc_execute(spawnd, SPAWND_REQ_SPAWN, &request, &res_data);

  if (res < 0) {
    panic("spawn error: %i", res);
  }

  printf("SPAWND LOG: spawned 'date' as [%08x]\n", res);
  wait(res);
};

void
check_spawn_echo(envid_t spawnd) {
  strcpy(request.env.file, "/echo");
  request.env.argc = 3;
  char arg0[] = "echo";
  char arg1[] = "Hello, ";
  char arg2[] = "world!";
  
  request.env.argv[0] = 0;
  request.env.argv[1] = sizeof(arg0);
  request.env.argv[2] = sizeof(arg0) + sizeof(arg1);

  memcpy(request.env.strtab, arg0, sizeof(arg0));
  memcpy(request.env.strtab + sizeof(arg0), arg1, sizeof(arg1));
  memcpy(request.env.strtab + sizeof(arg0) + sizeof(arg1), arg2, sizeof(arg2));

  printf("SPAWND CHECK: spawn echo\n");
  void* res_data = NULL;
  int res = rpc_execute(spawnd, SPAWND_REQ_SPAWN, &request, &res_data);

  if (res < 0) {
    panic("spawn error: %i", res);
  }

  printf("SPAWND LOG: spawned 'echo' as [%08x]\n", res);
  wait(res);
}

void
umain(int argc, char** argv) {
    envid_t initd = find_initd();

    envid_t spawnd = find_spawnd(initd);
    printf("SPAWND LOG: Found 'spawnd' in env [%08x]\n", spawnd);
    
    check_spawn_date(spawnd);
    check_spawn_echo(spawnd);
}
