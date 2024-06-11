/* See COPYRIGHT for copyright information. */
#include "inc/env.h"
#include "inc/mmu.h"
#include "inc/trap.h"
#include "inc/types.h"
#include "inc/uefi.h"
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/console.h>
#include <kern/env.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/pmap.h>
#include <kern/syscall.h>
#include <kern/trap.h>
#include <kern/traceopt.h>

#define CAT(x, y)           PRIMITIVE_CAT(x, y)
#define PRIMITIVE_CAT(x, y) x##y

#define PRINT_ARG(fmt, arg_name) \
    cprintf(#arg_name "=" fmt " ", (arg_name));

#define PRINT_ARGS(args)       CAT(PRINT_ARGS_1 args, _END)
#define PRINT_ARGS_1(fmt, arg) PRINT_ARG(fmt, arg) PRINT_ARGS_2
#define PRINT_ARGS_2(fmt, arg) PRINT_ARG(fmt, arg) PRINT_ARGS_1
#define PRINT_ARGS_1_END
#define PRINT_ARGS_2_END

#define TRACE_SYSCALL_ENTER(...)                                      \
    do {                                                              \
        if (trace_syscalls || trace_syscalls_from == curenv->env_id) { \
            cprintf("[%08x] ", curenv->env_id);                       \
            cprintf("called %s( ", __func__);                         \
            PRINT_ARGS(__VA_ARGS__);                                  \
            cprintf(")\n");                                           \
        }                                                             \
    } while (0)

#define TRACE_SYSCALL_LEAVE(fmt, ret)                                 \
    do {                                                              \
        if (trace_syscalls || trace_syscalls_from == curenv->env_id) { \
            cprintf("[%08x] ", curenv->env_id);                       \
            cprintf("returning " fmt " from %s(", (ret), __func__);   \
            cprintf(") at line %d\n", __LINE__);                      \
        }                                                             \
    } while (0)

#define TRACE_SYSCALL_NORETURN()                                      \
    do {                                                              \
        if (trace_syscalls || trace_syscalls_from == curenv->env_id) { \
            cprintf("[%08x] ", curenv->env_id);                       \
            cprintf("yielding from %s()\n", __func__);                \
        }                                                             \
    } while (0)

static inline int
abs(int val) {
    return val < 0 ? -val : val;
}

#define SYSCALL_ASSERT(condition, err_code)              \
    do {                                                 \
        if (!(condition)) {                              \
            TRACE_SYSCALL_LEAVE("'%i'", -abs(err_code)); \
            return -abs(err_code);                       \
        }                                                \
    } while (0)


/* Print a string to the system console.
 * The string is exactly 'len' characters long.
 * Destroys the environment on memory errors. */
static int
sys_cputs(const char* s, size_t len) {
#define ARGS ("%p", s)("%zu", len)
    TRACE_SYSCALL_ENTER(ARGS);
    /* Check that the user has permission to read memory [s, s+len).
     * Destroy the environment if not. */
    user_mem_assert(curenv, s, len, PROT_R | PROT_USER_);

    for (size_t i = 0; i < len; ++i) {
        cputchar(s[i]);
    }

    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
#undef ARGS
}

/* Read a character from the system console without blocking.
 * Returns the character, or 0 if there is no input waiting. */
static int
sys_cgetc(void) {
    TRACE_SYSCALL_ENTER();
    int ch = cons_getc();
    TRACE_SYSCALL_LEAVE("%d", ch);
    return ch;
}

/* Returns the current environment's envid. */
static envid_t
sys_getenvid(void) {
    TRACE_SYSCALL_ENTER();
    assert(curenv);
    TRACE_SYSCALL_LEAVE("%x", curenv->env_id);
    return curenv->env_id;
}

/* Destroy a given environment (possibly the currently running environment).
 *
 *  Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid. */
static int
sys_env_destroy(envid_t envid) {
#define ARGS ("%x", envid)
    TRACE_SYSCALL_ENTER(ARGS);
    struct Env* env = NULL;
    int result = envid2env(envid, &env, true);
    if (result != 0) {
        TRACE_SYSCALL_LEAVE("'%i'", result);
        return result;
    }
    if (trace_envs) {
        cprintf(env == curenv ?
                        "[%08x] exiting gracefully\n" :
                        "[%08x] destroying %08x\n",
                curenv->env_id, env->env_id);
    }
    bool curenv_destroyed = (env == curenv);
    if (curenv_destroyed) {
        TRACE_SYSCALL_NORETURN();
    }

    env_destroy(env);
    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
#undef ARGS
}

/* Deschedule current environment and pick a different one to run. */
static void
sys_yield(void) {
    TRACE_SYSCALL_ENTER();
    assert(curenv);
    TRACE_SYSCALL_NORETURN();
    sched_yield();
}

/* Allocate a new environment.
 * Returns envid of new environment, or < 0 on error.  Errors are:
 *  -E_NO_FREE_ENV if no free environment is available.
 *  -E_NO_MEM on memory exhaustion. */
static envid_t
sys_exofork(void) {
    /* Create the new environment with env_alloc(), from kern/env.c.
     * It should be left as env_alloc created it, except that
     * status is set to ENV_NOT_RUNNABLE, and the register set is copied
     * from the current environment -- but tweaked so sys_exofork
     * will appear to return 0. */
    TRACE_SYSCALL_ENTER();

    assert(curenv);
    struct Env* child_env = NULL;
    int alloc_res = env_alloc(&child_env, curenv->env_id, curenv->env_type);
    if (alloc_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", alloc_res);
        return alloc_res;
    }

    memcpy(&child_env->env_tf, &curenv->env_tf, sizeof(child_env->env_tf));
    child_env->env_tf.tf_regs.reg_rax = 0;
    child_env->env_status = ENV_NOT_RUNNABLE;

    TRACE_SYSCALL_LEAVE("%x", child_env->env_id);
    return child_env->env_id;
}

/* Set envid's env_status to status, which must be ENV_RUNNABLE
 * or ENV_NOT_RUNNABLE.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if status is not a valid status for an environment. */
static int
sys_env_set_status(envid_t envid, int status) {
    /* Hint: Use the 'envid2env' function from kern/env.c to translate an
     * envid to a struct Env.
     * You should set envid2env's third argument to 1, which will
     * check whether the current environment has permission to set
     * envid's status. */
#define ARGS ("%x", envid)("%x", status)
    TRACE_SYSCALL_ENTER(ARGS);
    if (status != ENV_NOT_RUNNABLE && status != ENV_RUNNABLE) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }

    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, curenv->env_type != ENV_TYPE_KERNEL);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", lookup_res);
        return lookup_res;
    }
    target->env_status = status;

    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
#undef ARGS
}

/* Set the page fault upcall for 'envid' by modifying the corresponding struct
 * Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
 * kernel will push a fault record onto the exception stack, then branch to
 * 'func'.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid. */
static int
sys_env_set_pgfault_upcall(envid_t envid, void* func) {
    struct Env* target = NULL;
    int result = envid2env(envid, &target, true);
    if (result < 0)
        return -E_BAD_ENV;

    target->env_pgfault_upcall = func;
    return 0;
}

/* Allocate a region of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 * The page's contents are set to 0.
 * If a page is already mapped at 'va', that page is unmapped as a
 * side effect.
 *
 * This call should work with or without ALLOC_ZERO/ALLOC_ONE flags
 * (set them if they are not already set)
 *
 * It allocates memory lazily so you need to use map_region
 * with PROT_LAZY and ALLOC_ONE/ALLOC_ZERO set.
 *
 * Don't forget to set PROT_USER_
 *
 * PROT_ALL is useful for validation.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned.
 *  -E_INVAL if perm is inappropriate (see above).
 *  -E_NO_MEM if there's no memory to allocate the new page,
 *      or to allocate any necessary page tables. */
static int
sys_alloc_region(envid_t envid, uintptr_t addr, size_t size, int perm) {
#define ARGS ("%x", envid)("0x%lx", addr)("0x%zx", size)("%x", perm)
    TRACE_SYSCALL_ENTER(ARGS);
    if (addr >= MAX_USER_ADDRESS || (addr & CLASS_MASK(0))) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }
    if ((perm & ~(ALLOC_ONE | ALLOC_ZERO)) != (perm & PROT_ALL)) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }
    if ((perm & ALLOC_ZERO) && (perm & ALLOC_ONE)) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }

    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, curenv->env_type != ENV_TYPE_KERNEL);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", lookup_res);
        return lookup_res;
    }

    if (!(perm & ALLOC_ONE)) {
        perm |= ALLOC_ZERO;
    }

    perm |= PROT_USER_ | PROT_LAZY;

    int map_res = map_region(&target->address_space, addr, NULL, 0, size, perm);
    if (map_res != 0) {
        TRACE_SYSCALL_LEAVE("'%i'", map_res);
    } else {
        TRACE_SYSCALL_LEAVE("%d", 0);
    }
    return map_res;
#undef ARGS
}

/* Map the region of memory at 'srcva' in srcenvid's address space
 * at 'dstva' in dstenvid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_alloc_region, except
 * that it also does not supprt ALLOC_ONE/ALLOC_ONE flags.
 *
 * You only need to check alignment of addresses, perm flags and
 * that addresses are a part of user space. Everything else is
 * already checked inside map_region().
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
 *      or the caller doesn't have permission to change one of them.
 *  -E_INVAL if srcva >= MAX_USER_ADDRESS or srcva is not page-aligned,
 *      or dstva >= MAX_USER_ADDRESS or dstva is not page-aligned.
 *  -E_INVAL is srcva is not mapped in srcenvid's address space.
 *  -E_INVAL if perm is inappropriate (see sys_page_alloc).
 *  -E_INVAL if (perm & PROT_W), but srcva is read-only in srcenvid's
 *      address space.
 *  -E_NO_MEM if there's no memory to allocate any necessary page tables. */

static int
sys_map_region(envid_t srcenvid, uintptr_t srcva,
               envid_t dstenvid, uintptr_t dstva, size_t size, int perm) {
#define ARGS ("%x", srcenvid)("0x%lx", srcva)("%x", dstenvid)("0x%lx", dstva)("0x%zx", size)("%x", perm)
    TRACE_SYSCALL_ENTER(ARGS);
    if (srcva >= MAX_USER_ADDRESS || (srcva & CLASS_MASK(0))) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }
    if (dstva >= MAX_USER_ADDRESS || (dstva & CLASS_MASK(0))) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }
    if (perm != (perm & PROT_ALL)) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }

    int lookup_res = 0;
    struct Env* src = NULL;
    struct Env* dst = NULL;
    lookup_res = envid2env(srcenvid, &src, curenv->env_type != ENV_TYPE_KERNEL);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", lookup_res);
        return lookup_res;
    }
    lookup_res = envid2env(dstenvid, &dst, curenv->env_type != ENV_TYPE_KERNEL);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", lookup_res);
        return lookup_res;
    }

    perm |= PROT_USER_;

    /*
    if (user_mem_check(src, (const void*)srcva, size, (perm & PROT_RWX) | PROT_USER_) != 0) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }
    */


    int map_res = map_region(&dst->address_space, dstva, &src->address_space,
                             srcva, size, perm);
    if (map_res != 0) {
        TRACE_SYSCALL_LEAVE("'%i'", map_res);
    } else {
        TRACE_SYSCALL_LEAVE("%d", 0);
    }
    return map_res;
#undef ARGS
}

/* Unmap the region of memory at 'va' in the address space of 'envid'.
 * If no page is mapped, the function silently succeeds.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned. */
static int
sys_unmap_region(envid_t envid, uintptr_t va, size_t size) {
#define ARGS ("%x", envid)("0x%lx", va)("%zu", size)
    TRACE_SYSCALL_ENTER(ARGS);
    /* Hint: This function is a wrapper around unmap_region(). */
    if (va >= MAX_USER_ADDRESS || (va & CLASS_MASK(0))) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
        return -E_INVAL;
    }

    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, curenv->env_type != ENV_TYPE_KERNEL);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", lookup_res);
        return lookup_res;
    }

    unmap_region(&target->address_space, va, size);
    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
#undef ARGS
}

/* Map region of physical memory to the userspace address.
 * This is meant to be used by the userspace drivers, of which
 * the only one currently is the filesystem server.
 *
 * Return 0 on succeeds, < 0 on error. Erros are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_BAD_ENV if is not a filesystem driver (ENV_TYPE_FS).
 *  -E_INVAL if va >= MAX_USER_ADDRESS, or va is not page-aligned.
 *  -E_INVAL if pa is not page-aligned.
 *  -E_INVAL if size is not page-aligned.
 *  -E_INVAL if prem contains invalid flags
 *     (including PROT_SHARE, PROT_COMBINE or PROT_LAZY).
 *  -E_NO_MEM if address does not exist.
 *  -E_NO_ENT if address is already used. */
static int
sys_map_physical_region(uintptr_t pa, envid_t envid, uintptr_t va, size_t size, int perm) {
    // TIP: Use map_physical_region() with (perm | PROT_USER_ | MAP_USER_MMIO)
    //      And don't forget to validate arguments as always.
    TRACE_SYSCALL_ENTER(("0x%lx", pa)("%x", envid)("0x%lx", va)("0x%zx", size)("%x", perm));
    SYSCALL_ASSERT(curenv->env_type == ENV_TYPE_FS
                || curenv->env_type == ENV_TYPE_KERNEL, E_BAD_ENV);
    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, true);
    SYSCALL_ASSERT(lookup_res == 0, E_BAD_ENV);
    SYSCALL_ASSERT(target->env_type == ENV_TYPE_FS
                || target->env_type == ENV_TYPE_KERNEL, E_BAD_ENV);
    SYSCALL_ASSERT(va < MAX_USER_ADDRESS, E_INVAL);
    SYSCALL_ASSERT(va % PAGE_SIZE == 0, E_INVAL);
    SYSCALL_ASSERT(pa % PAGE_SIZE == 0, E_INVAL);
    int allowed_flags = PROT_RWX | PROT_WC | PROT_CD;
    SYSCALL_ASSERT((perm & ~allowed_flags) == 0, E_INVAL);

    int map_res = map_physical_region(&target->address_space,
                                      va, pa, size, perm | PROT_USER_ | MAP_USER_MMIO);
    SYSCALL_ASSERT(map_res == 0, map_res);

    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
}

/* Try to send 'value' to the target env 'envid'.
 * If srcva < MAX_USER_ADDRESS, then also send region currently mapped at 'srcva',
 * so that receiver gets mapping.
 *
 * The send fails with a return value of -E_IPC_NOT_RECV if the
 * target is not blocked, waiting for an IPC.
 *
 * The send also can fail for the other reasons listed below.
 *
 * Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends;
 *    env_ipc_maxsz is set to min of size and it's current vlaue;
 *    env_ipc_from is set to the sending envid;
 *    env_ipc_value is set to the 'value' parameter;
 *    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
 * The target environment is marked runnable again, returning 0
 * from the paused sys_ipc_recv system call.  (Hint: does the
 * sys_ipc_recv function ever actually return?)
 *
 * If the sender wants to send a page but the receiver isn't asking for one,
 * then no page mapping is transferred, but no error occurs.
 * The ipc only happens when no errors occur.
 * Send region size is the minimum of sized specified in sys_ipc_try_send() and sys_ipc_recv()
 *
 * Returns 0 on success, < 0 on error.
 * Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist.
 *      (No need to check permissions.)
 *  -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
 *      or another environment managed to send first.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not page-aligned.
 *  -E_INVAL if srcva < MAX_USER_ADDRESS and perm is inappropriate
 *      (see sys_page_alloc).
 *  -E_INVAL if srcva < MAX_USER_ADDRESS but srcva is not mapped in the caller's
 *      address space.
 *  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
 *      current environment's address space.
 *  -E_NO_MEM if there's not enough memory to map srcva in envid's
 *      address space. */
static int
sys_ipc_try_send(envid_t envid, uint32_t value, uintptr_t srcva, size_t size, int perm) {
#define ARGS ("%x", envid)("%u", value)("0x%lx", srcva)("0x%lx", size)("%x", perm)
    TRACE_SYSCALL_ENTER(ARGS);
    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, false);
    if (lookup_res < 0) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_BAD_ENV);
        return -E_BAD_ENV;
    }

    if (!target->env_ipc_recving) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_IPC_NOT_RECV);
        return -E_IPC_NOT_RECV;
    }
    if (target->env_ipc_from != 0 && target->env_ipc_from != curenv->env_id) {
        TRACE_SYSCALL_LEAVE("'%i'", -E_IPC_NOT_RECV);
        return -E_IPC_NOT_RECV;
    }

    if (srcva < MAX_USER_ADDRESS && target->env_ipc_dstva < MAX_USER_ADDRESS) {
        if (srcva & CLASS_MASK(0)) {
            TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
            return -E_INVAL;
        }
        if (perm != (perm & PROT_ALL)) {
            TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
            return -E_INVAL;
        }
        size_t sent_size = size < target->env_ipc_maxsz ? size : target->env_ipc_maxsz;

        int map_res = map_region(&target->address_space, target->env_ipc_dstva,
                                 &curenv->address_space, srcva,
                                 sent_size, perm | PROT_USER_);
        if (map_res != 0) {
            TRACE_SYSCALL_LEAVE("'%i'", map_res);
            return map_res;
        }
        if (trace_syscalls) {
            cprintf("[%08x] %s: Sent region %p of size 0x%lx to %p in [%x]\n",
                    curenv->env_id, __func__,
                    (void*)srcva, sent_size, (void*)target->env_ipc_dstva,
                    envid);
        }

        target->env_ipc_perm = perm;
        target->env_ipc_maxsz = size;
    }
    target->env_ipc_value = value;
    target->env_ipc_from = curenv->env_id;
    target->env_ipc_recving = false;

    target->env_tf.tf_regs.reg_rax = 0;
    target->env_status = ENV_RUNNABLE;

    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
#undef ARGS
}

/* Block until a value is ready.  Record that you want to receive
 * using the env_ipc_recving, env_ipc_maxsz and env_ipc_dstva fields of struct Env,
 * mark yourself not runnable, and then give up the CPU.
 *
 * If 'dstva' is < MAX_USER_ADDRESS, then you are willing to receive a page of data.
 * 'dstva' is the virtual address at which the sent page should be mapped.
 *
 * This function only returns on error, but the system call will eventually
 * return 0 on success.
 * Return < 0 on error.  Errors are:
 *  -E_INVAL if dstva < MAX_USER_ADDRESS but dstva is not page-aligned;
 *  -E_INVAL if dstva is valid and maxsize is 0,
 *  -E_INVAL if maxsize is not page aligned. */
static int
sys_ipc_recv(envid_t from, uintptr_t dstva, uintptr_t maxsize) {
#define ARGS ("%08x", from)("0x%lx", dstva)("0x%lx", maxsize)
    TRACE_SYSCALL_ENTER(ARGS);
    if (dstva < MAX_USER_ADDRESS) {
        if ((dstva & CLASS_MASK(0)) || !maxsize || (maxsize & CLASS_MASK(0))) {
            TRACE_SYSCALL_LEAVE("'%i'", -E_INVAL);
            return -E_INVAL;
        }
        curenv->env_ipc_dstva = dstva;
        curenv->env_ipc_maxsz = maxsize;
    }

    curenv->env_ipc_from = from;
    curenv->env_ipc_perm = 0;
    curenv->env_ipc_value = 0;

    curenv->env_ipc_recving = true;
    curenv->env_status = ENV_NOT_RUNNABLE;

    TRACE_SYSCALL_NORETURN();
    sched_yield();
    return 0;
#undef ARGS
}

/*
 * This function sets trapframe and is unsafe
 * so you need:
 *   -Check environment id to be valid and accessible
 *   -Check argument to be valid memory
 *   -Use nosan_memcpy to copy from usespace
 *   -Prevent privilege escalation by overriding segments
 *   -Only allow program to set safe flags in RFLAGS register
 *   -Force IF to be set in RFLAGS
 */
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe* tf) {
    TRACE_SYSCALL_ENTER(("%08x", envid)("%p", tf));
    user_mem_assert(curenv, tf, sizeof(*tf), PROT_R | PROT_USER_);

    struct Trapframe tf_copy;
    nosan_memcpy(&tf_copy, tf, sizeof(tf_copy));

    tf_copy.tf_ds = GD_UD | 3;
    tf_copy.tf_es = GD_UD | 3;
    tf_copy.tf_ss = GD_UD | 3;
    tf_copy.tf_cs = GD_UT | 3;

    tf_copy.tf_rflags &= ~FL_SYSTEM;
    tf_copy.tf_rflags |= FL_IOPL_3 | FL_IF;

    struct Env* target = NULL;
    int lookup_res = envid2env(envid, &target, curenv->env_type != ENV_TYPE_KERNEL);
    SYSCALL_ASSERT(lookup_res == 0, E_BAD_ENV);

    memcpy(&target->env_tf, &tf_copy, sizeof(tf_copy));

    TRACE_SYSCALL_LEAVE("%d", 0);
    return 0;
}

/* Return date and time in UNIX timestamp format: seconds passed
 * from 1970-01-01 00:00:00 UTC. */
static int
sys_gettime(void) {
    TRACE_SYSCALL_ENTER();
    int time = gettime();
    TRACE_SYSCALL_LEAVE("%d", time);
    return time;
}

/*
 * This function return the difference between maximal
 * number of references of regions [addr, addr + size] and [addr2,addr2+size2]
 * if addr2 is less than MAX_USER_ADDRESS, or just
 * maximal number of references to [addr, addr + size]
 *
 * Use region_maxref() here.
 */
static int
sys_region_refs(uintptr_t addr, size_t size, uintptr_t addr2, size_t size2) {
#define ARGS ("0x%lx", addr)("0x%zx", size)("0x%lx", addr2)("0x%zx", size2)
    TRACE_SYSCALL_ENTER(ARGS);
    SYSCALL_ASSERT(addr < MAX_USER_ADDRESS, E_INVAL);
    SYSCALL_ASSERT(size > 0, E_INVAL);

    int refs1 = region_maxref(&curenv->address_space, addr, size);
    int refs2 = 0;

    if (addr2 < MAX_USER_ADDRESS) {
        SYSCALL_ASSERT(size2 > 0, E_INVAL);
        refs2 = region_maxref(&curenv->address_space, addr2, size2);
    }

    TRACE_SYSCALL_LEAVE("%d", refs1 - refs2);
    return refs1 - refs2;
#undef ARGS
}

static int
sys_get_rsdp_paddr(physaddr_t* phys_addr) {
  TRACE_SYSCALL_ENTER(("%p", phys_addr));
  SYSCALL_ASSERT(curenv->env_type == ENV_TYPE_KERNEL, E_BAD_ENV);
  user_mem_assert(curenv, phys_addr, sizeof(*phys_addr), PROT_USER_ | PROT_R | PROT_W);

  physaddr_t root = 0;
  struct AddressSpace* old = switch_address_space(&kspace);
  root = uefi_lp->ACPIRoot;
  switch_address_space(old);

  *phys_addr = root;
  TRACE_SYSCALL_LEAVE("%d", 0);
  return 0;
}

/* Dispatches to the correct kernel function, passing the arguments. */
uintptr_t
syscall(uintptr_t syscallno, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
    /* Call the function corresponding to the 'syscallno' parameter.
     * Return any appropriate return value. */

    switch (syscallno) {
    case SYS_cputs:
        sys_cputs((const char*)a1, a2);
        return 0;
    case SYS_cgetc:
        return sys_cgetc();
    case SYS_getenvid:
        return sys_getenvid();
    case SYS_env_destroy:
        return sys_env_destroy(a1);
    case SYS_alloc_region:
        return sys_alloc_region(a1, a2, a3, a4);
    case SYS_map_region:
        return sys_map_region(a1, a2, a3, a4, a5, a6);
    case SYS_map_physical_region:
        return sys_map_physical_region(a1, a2, a3, a4, a5);
    case SYS_unmap_region:
        return sys_unmap_region(a1, a2, a3);
    case SYS_region_refs:
        return sys_region_refs(a1, a2, a3, a4);
    case SYS_exofork:
        return sys_exofork();
    case SYS_env_set_status:
        return sys_env_set_status(a1, a2);
    case SYS_env_set_trapframe:
        return sys_env_set_trapframe(a1, (struct Trapframe*)a2);
    case SYS_env_set_pgfault_upcall:
        return sys_env_set_pgfault_upcall(a1, (void*)a2);
    case SYS_yield:
        sys_yield();
        panic("Unreachable");
        return -E_INVAL;
    case SYS_ipc_try_send:
        return sys_ipc_try_send(a1, a2, a3, a4, a5);
    case SYS_ipc_recv:
        return sys_ipc_recv(a1, a2, a3);
    case SYS_gettime:
        return sys_gettime();
    case SYS_get_rsdp_paddr:
        return sys_get_rsdp_paddr((physaddr_t*)a1);
    case NSYSCALLS:
    default:
        return -E_NO_SYS;
    }
    // LAB 12: Your code here

    return -E_NO_SYS;
}
