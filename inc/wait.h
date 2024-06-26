#pragma once

#include <inc/types.h>

#define WNOHANG 0xBAD

/* If WIFEXITED(STATUS), the low-order 8 bits of the status.  */
#define __WEXITSTATUS(status) (((status) & 0xff00) >> 8)

/* If WIFSIGNALED(STATUS), the terminating signal.  */
#define __WTERMSIG(status) ((status) & 0x7f)

/* If WIFSTOPPED(STATUS), the signal that stopped the child.  */
#define __WSTOPSIG(status) __WEXITSTATUS(status)

/* Nonzero if STATUS indicates termination by a signal.  */
#define __WIFSIGNALED(status) \
    (((signed char)(((status) & 0x7f) + 1) >> 1) > 0)

/* Nonzero if STATUS indicates normal termination.  */
#define __WIFEXITED(status) (__WTERMSIG(status) == 0)

#define WEXITSTATUS(status) __WEXITSTATUS(status)
#define WIFSIGNALED(status) __WIFSIGNALED(status)
#define WTERMSIG(status)    __WTERMSIG(status)
#define WIFEXITED(status)   __WIFEXITED(status)


/* wait.c */
void wait(envid_t env);

pid_t waitpid(pid_t pid, int * wstatus, int options);
