/* See COPYRIGHT for copyright information. */

#include <inc/syscall.h>
#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/console.h>
#include <kern/env.h>
#include <kern/kclock.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/syscall.h>
#include <kern/trap.h>
#include <kern/traceopt.h>

/* Print a string to the system console.
 * The string is exactly 'len' characters long.
 * Destroys the environment on memory errors. */
static int
sys_cputs(const char *s, size_t len) {
    if (trace_syscalls) {
      cprintf("called sys_cputs(s=%p, len=%lu)\n", s, len);
    }
    /* Check that the user has permission to read memory [s, s+len).
     * Destroy the environment if not. */
    user_mem_assert(curenv, s, len, PROT_R | PROT_USER_);

    for (size_t i = 0; i < len; ++i)
    {
      cputchar(s[i]);
    }

    if (trace_syscalls) {
      cprintf("returning 0 from sys_cputs(s=%p, len=%lu)\n", s, len);
    }
    return 0;
}

/* Read a character from the system console without blocking.
 * Returns the character, or 0 if there is no input waiting. */
static int
sys_cgetc(void) {
    if (trace_syscalls) {
      cprintf("called sys_cgetc()\n");
    }
    int ch = cons_getc();
    if (trace_syscalls) {
      cprintf("returning %d from sys_cgetc()\n", ch);
    }
    return ch;
}

/* Returns the current environment's envid. */
static envid_t
sys_getenvid(void) {
    if (trace_syscalls) {
      cprintf("called sys_getenvid()\n");
    }
    assert(curenv);
    if (trace_syscalls) {
      cprintf("returning %x from sys_getenvid()\n", curenv->env_id);
    }
    return curenv->env_id;
}

/* Destroy a given environment (possibly the currently running environment).
 *
 *  Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid. */
static int
sys_env_destroy(envid_t envid) {
    if (trace_syscalls) {
      cprintf("called sys_env_destroy(envid=%x)\n", envid);
    }
    struct Env* env = NULL;
    int result = envid2env(envid, &env, true);
    if (result != 0) {
      if (trace_syscalls) {
        cprintf("returning %i sys_env_destroy(envid=%d)\n", result, envid);
      }
      return -E_BAD_ENV;
    }
    if (trace_envs) {
        cprintf(env == curenv ?
                        "[%08x] exiting gracefully\n" :
                        "[%08x] destroying %08x\n",
                curenv->env_id, env->env_id);
    }
    bool curenv_destroyed = (env == curenv);
    env_destroy(env);
    if (!curenv_destroyed && trace_syscalls) {
      cprintf("returning 0 sys_env_destroy(envid=%x)\n", envid);
    }
    return 0;
}

/* Dispatches to the correct kernel function, passing the arguments. */
uintptr_t
syscall(uintptr_t syscallno, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6) {
    /* Call the function corresponding to the 'syscallno' parameter.
     * Return any appropriate return value. */

    switch (syscallno)
    {
    case SYS_cputs:
      sys_cputs((const char*) a1, a2);
      return 0;
    case SYS_cgetc:
      return sys_cgetc();
    case SYS_getenvid:
      return sys_getenvid();
    case SYS_env_destroy:
      return sys_env_destroy(a1);
    case NSYSCALLS:
    default:
      return -E_NO_SYS;
    }

    return -E_NO_SYS;
}
