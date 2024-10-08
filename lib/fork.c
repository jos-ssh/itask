/* implement fork from user space */

#include "inc/kmod/file.h"
#include "inc/rpc.h"
#include <inc/env.h>
#include <inc/string.h>
#include <inc/lib.h>

/* User-level fork with copy-on-write.
 * Create a child.
 * Lazily copy our address space and page fault handler setup to the child.
 * Then mark the child as runnable and return.
 *
 * Returns: child's envid to the parent, 0 to the child, < 0 on error.
 * It is also OK to panic on error.
 *
 * Hint:
 *   Use sys_map_region, it can perform address space copying in one call
 *   Don't forget to set page fault handler in the child (using sys_env_set_pgfault_upcall()).
 *   Remember to fix "thisenv" in the child process.
 */
envid_t
fork(void) {
    static envid_t filed = 0;
    if (!filed) {
        filed = kmod_find_any_version(FILED_MODNAME);
    }

    rpc_execute(filed, FILED_REQ_FORK, NULL, NULL);
    thisenv = &envs[ENVX(sys_getenvid())];
    return thisenv->env_ipc_value;
#if 0
    void* parent_upcall = thisenv->env_pgfault_upcall;
    envid_t child = sys_exofork();

    if (child < 0)
      return child;
    
    if (child == 0) {
        if (parent_upcall)
          sys_env_set_pgfault_upcall(CURENVID, parent_upcall);

        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    sys_map_region(0, NULL, child, NULL,
        MAX_USER_ADDRESS, PROT_ALL | PROT_LAZY | PROT_COMBINE);

    int status_res = sys_env_set_status(child, ENV_RUNNABLE);
    if (status_res < 0) {
      return status_res;
    }
    return child;
#endif
}

envid_t
sfork() {
    panic("sfork() is not implemented");
}
