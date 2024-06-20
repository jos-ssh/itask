#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

static union InitdRequest request;

void
check_spawn_date(envid_t initd) {
    request.spawn.parent = 0;
    strcpy(request.spawn.file, "/date");
    request.spawn.argc = 0;

    printf("INITD CHECK: spawn date\n");
    void* res_data = NULL;
    int res = rpc_execute(initd, INITD_REQ_SPAWN, &request, &res_data);

    if (res < 0) {
        panic("spawn error: %i", res);
    }
    envid_t child = res;

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) {
        panic("status error: %i", res);
    }

    printf("INITD LOG: spawned 'date' as [%08x]\n", child);
    wait(child);
};

void
check_spawn_echo(envid_t initd) {
    request.spawn.parent = 0;
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

    envid_t child = res;
    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) {
        panic("status error: %i", res);
    }

    printf("INITD LOG: spawned 'echo' as [%08x]\n", child);
    wait(child);
}

void
check_fork(envid_t initd) {
    void* res_data = NULL;
    printf("INITD CHECK: fork\n");
    envid_t parent = thisenv->env_id;

    request.fork.parent = 0;

    int res = rpc_execute(initd, INITD_REQ_FORK, &request, &res_data);
    // fixup thisenv
    thisenv = &envs[ENVX(sys_getenvid())];
    res = thisenv->env_ipc_value;

    if (res < 0) {
        panic("spawn error: %i", res);
    }

    assert(res != sys_getenvid());
    if (res == 0) {
        printf("INITD LOG: child running\n");
        thisenv = &envs[ENVX(sys_getenvid())];

        assert(thisenv->env_id != parent);
        assert(thisenv->env_parent_id == parent);
        assert(thisenv->env_type == ENV_TYPE_USER);
        return;
    }

    envid_t child = res;
    assert(envs[ENVX(child)].env_ipc_value == 0);

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) {
        panic("status error: %i", res);
    }

    printf("INITD LOG: forked child [%08x]\n", child);
    wait(child);
}

void
umain(int argc, char** argv) {
    envid_t initd = kmod_find_any_version(INITD_MODNAME);
    assert(initd > 0);
    cprintf("INITD LOG: Found 'initd' in env [%08x]\n", initd);

    check_spawn_date(initd);
    check_spawn_echo(initd);
    check_fork(initd);
}
