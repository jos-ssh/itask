/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ENV_H
#define JOS_INC_ENV_H

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>
#include <inc/assert.h>

typedef int32_t envid_t;

/* An environment ID 'envid_t' has three parts:
 *
 * +1+---------------21-----------------+--------10--------+
 * |0|          Uniqueifier             |   Environment    |
 * | |                                  |      Index       |
 * +------------------------------------+------------------+
 *                                       \--- ENVX(eid) --/
 *
 * The environment index ENVX(eid) equals the environment's offset in the
 * 'envs[]' array.  The uniqueifier distinguishes environments that were
 * created at different times, but share the same environment index.
 *
 * All real environments are greater than 0 (so the sign bit is zero).
 * envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
 * stands for the current environment.
 */

#define LOG2NENV    10
#define NENV        (1 << LOG2NENV)
#define ENVX(envid) ((envid) & (NENV - 1))

/* Values of env_status in struct Env */
enum {
    ENV_FREE = 0,
    ENV_DYING = 1,
    ENV_RUNNABLE = 2,
    ENV_RUNNING = 3,
    ENV_NOT_RUNNABLE = 4
};

/* Bits used by scheduler */
#define ENV_SCHED_STATUS_MASK 0x7

__attribute__((always_inline)) static inline unsigned
env_set_sched_status(unsigned *old_status, unsigned new_status) {
    assert(new_status <= ENV_SCHED_STATUS_MASK);
    unsigned saved_status = *old_status;
    if (new_status == ENV_FREE) {
        *old_status = ENV_FREE;
    } else {
        *old_status &= ~ENV_SCHED_STATUS_MASK;
        *old_status |= new_status;
    }
    return saved_status;
}

__attribute__((always_inline)) static inline bool
env_check_sched_status(unsigned status, unsigned expected) {
    return (status & ENV_SCHED_STATUS_MASK) == expected;
}

/* Special environment types */
enum EnvType {
    ENV_TYPE_IDLE,
    ENV_TYPE_KERNEL,
    ENV_TYPE_USER,
    ENV_TYPE_FS, /* File system server */
};

struct AddressSpace {
    pml4e_t *pml4;     /* Virtual address of pml4 */
    uintptr_t cr3;     /* Physical address of pml4 */
    struct Page *root; /* root node of address space tree */
};


struct Env {
    struct Trapframe env_tf; /* Saved registers */
    struct Env *env_link;    /* Next free Env */
    envid_t env_id;          /* Unique environment identifier */
    envid_t env_parent_id;   /* env_id of this env's parent */
    enum EnvType env_type;   /* Indicates special system environments */
    unsigned env_status;     /* Status of the environment */
    uint32_t env_runs;       /* Number of times environment has run */

    uint8_t *binary; /* Pointer to process ELF image in kernel memory */

    /* Address space */
    struct AddressSpace address_space;

    /* Exception handling */
    void *env_pgfault_upcall; /* Page fault upcall entry point */

    /* LAB 9 IPC */
    bool env_ipc_recving;    /* Env is blocked receiving */
    uintptr_t env_ipc_dstva; /* VA at which to map received page */
    size_t env_ipc_maxsz;    /* maximal size of received region */
    uint32_t env_ipc_value;  /* Data value sent to us */
    envid_t env_ipc_from;    /* envid of the sender */
    int env_ipc_perm;        /* Perm of page mapping received */
};

#endif /* !JOS_INC_ENV_H */
