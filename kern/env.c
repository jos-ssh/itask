/* See COPYRIGHT for copyright information. */
#include "inc/env.h"
#include "inc/memlayout.h"
#include "inc/stdio.h"
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/elf.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

#include <kern/env.h>
#include <kern/kdebug.h>
#include <kern/macro.h>
#include <kern/monitor.h>
#include <kern/pmap.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/timer.h>
#include <kern/traceopt.h>
#include <kern/trap.h>

/* Currently active environment */
struct Env *curenv = NULL;

#if 0 /* CONFIG_KSPACE */
/* All environments */
struct Env env_array[NENV];
struct Env *envs = env_array;
#else
/* All environments */
struct Env *envs = NULL;
#endif

/* Free environment list
 * (linked by Env->env_link) */
static struct Env *env_free_list;


/* NOTE: Should be at least LOGNENV */
#define ENVGENSHIFT 12

#if 1
#define COND_VERIFY(cond, error) \
    if (!(cond)) return -(error)
#else
#define COND_VERIFY(cond, error) assert(cond)
#endif

/* Converts an envid to an env pointer.
 * If checkperm is set, the specified environment must be either the
 * current environment or an immediate child of the current environment.
 *
 * RETURNS
 *     0 on success, -E_BAD_ENV on error.
 *   On success, sets *env_store to the environment.
 *   On error, sets *env_store to NULL. */
int
envid2env(envid_t envid, struct Env **env_store, bool need_check_perm) {
    struct Env *env;

    /* If envid is zero, return the current environment. */
    if (!envid) {
        *env_store = curenv;
        return 0;
    }

    /* Look up the Env structure via the index part of the envid,
     * then check the env_id field in that struct Env
     * to ensure that the envid is not stale
     * (i.e., does not refer to a _previous_ environment
     * that used the same slot in the envs[] array). */
    env = &envs[ENVX(envid)];
    if (env->env_status == ENV_FREE || env->env_id != envid) {
        *env_store = NULL;
        return -E_BAD_ENV;
    }

    /* Check that the calling environment has legitimate permission
     * to manipulate the specified environment.
     * If checkperm is set, the specified environment
     * must be either the current environment
     * or an immediate child of the current environment. */
    if (need_check_perm && env != curenv && env->env_parent_id != curenv->env_id) {
        *env_store = NULL;
        return -E_BAD_ENV;
    }

    *env_store = env;
    return 0;
}

/* Mark all environments in 'envs' as free, set their env_ids to 0,
 * and insert them into the env_free_list.
 * Make sure the environments are in the free list in the same order
 * they are in the envs array (i.e., so that the first call to
 * env_alloc() returns envs[0]).
 */
void
env_init(void) {
#if 0 /* CONFIG_KSPACE */
    const size_t env_count = NENV;
#else
    /* kzalloc_region only works with current_space != NULL */
    struct AddressSpace *cur_space = switch_address_space(&kspace);

    /* Allocate envs array with kzalloc_region().
     ;* Don't forget about rounding.
     * kzalloc_region() only works with current_space != NULL */
    envs = kzalloc_region(UENVS_SIZE);
    assert(envs);
    switch_address_space(cur_space);

    /* Map envs to UENVS read-only,
     * but user-accessible (with PROT_USER_ set) */
    map_region(&kspace, UENVS, &kspace, (uintptr_t)envs,
               UENVS_SIZE, PROT_R | PROT_USER_);

    static_assert(UENVS_SIZE / sizeof(*envs), "Not ehough space for envs");
    const size_t env_count = UENVS_SIZE / sizeof(*envs);
#endif // CONFIG_KSPACE

    /* Set up envs array */
    for (size_t env_idx = 0; env_idx < env_count; ++env_idx) {
        envs[env_idx].env_link =
                env_idx + 1 < env_count ? envs + env_idx + 1 : NULL;
        envs[env_idx].env_status = ENV_FREE;
    }
    env_free_list = envs;

}

/* Allocates and initializes a new environment.
 * On success, the new environment is stored in *newenv_store.
 *
 * Returns
 *     0 on success, < 0 on failure.
 * Errors
 *    -E_NO_FREE_ENV if all NENVS environments are allocated
 *    -E_NO_MEM on memory exhaustion
 */
int
env_alloc(struct Env **newenv_store, envid_t parent_id, enum EnvType type) {

    struct Env *env;
    if (!(env = env_free_list))
        return -E_NO_FREE_ENV;

    /* Allocate and set up the page directory for this environment. */
    int res = init_address_space(&env->address_space);
    if (res < 0) return res;

    /* Generate an env_id for this environment */
    int32_t generation = (env->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
    /* Don't create a negative env_id */
    if (generation <= 0) generation = 1 << ENVGENSHIFT;
    env->env_id = generation | (env - envs);

    /* Set the basic status variables */
    env->env_parent_id = parent_id;
#if 0 /* CONFIG_KSPACE */
    env->env_type = ENV_TYPE_KERNEL;
#else
    env->env_type = type;
#endif
    env->env_status = ENV_RUNNABLE;
    env->env_runs = 0;

    /* Clear out all the saved register state,
     * to prevent the register values
     * of a prior environment inhabiting this Env structure
     * from "leaking" into our new environment */
    memset(&env->env_tf, 0, sizeof(env->env_tf));

    /* Set up appropriate initial values for the segment registers.
     * GD_UD is the user data (KD - kernel data) segment selector in the GDT, and
     * GD_UT is the user text (KT - kernel text) segment selector (see inc/memlayout.h).
     * The low 2 bits of each segment register contains the
     * Requestor Privilege Level (RPL); 3 means user mode, 0 - kernel mode.  When
     * we switch privilege levels, the hardware does various
     * checks involving the RPL and the Descriptor Privilege Level
     * (DPL) stored in the descriptors themselves */

#if 0 /* CONFIG_KSPACE */
    env->env_tf.tf_ds = GD_KD;
    env->env_tf.tf_es = GD_KD;
    env->env_tf.tf_ss = GD_KD;
    env->env_tf.tf_cs = GD_KT;

    static uintptr_t stack_top = 0x2000000;

    // Allocate user stack
    stack_top += 2 * PAGE_SIZE;
    if (stack_top <= 0x2000000) {
        panic("Kernel out of stack memory");
    }

    env->env_tf.tf_rsp = stack_top;
#else
    env->env_tf.tf_ds = GD_UD | 3;
    env->env_tf.tf_es = GD_UD | 3;
    env->env_tf.tf_ss = GD_UD | 3;
    env->env_tf.tf_cs = GD_UT | 3;
    env->env_tf.tf_rsp = USER_STACK_TOP;

    // Allocate stack
    int stack_status = map_region(
            &env->address_space, USER_STACK_TOP - USER_STACK_SIZE,
            NULL, 0,
            USER_STACK_SIZE, PROT_R | PROT_W | PROT_USER_ | ALLOC_ZERO);
    if (stack_status < 0) {
        return stack_status;
    }
    
    // Unposon allocated user stack
#ifdef SANITIZE_SHADOW_BASE
    struct AddressSpace* old_space = switch_address_space(&env->address_space);
    platform_asan_unpoison((void*)(USER_STACK_TOP - PAGE_SIZE), PAGE_SIZE);
    switch_address_space(old_space);
#endif // SANITIZE_SHADOW_BASE

#endif

    /* For now init trapframe with IF set */
    env->env_tf.tf_rflags = FL_IF | (type == ENV_TYPE_FS ? FL_IOPL_3 : FL_IOPL_0);

    /* Clear the page fault handler until user installs one. */
    env->env_pgfault_upcall = 0;

    /* Also clear the IPC receiving flag. */
    env->env_ipc_recving = 0;

    /* Commit the allocation */
    env_free_list = env->env_link;
    *newenv_store = env;

    if (trace_envs) {
        cprintf("[%08x] new env %08x\n",
                curenv ? curenv->env_id : 0,
                env->env_id);
    }
    return 0;
}

/* Pass the original ELF image to binary/size and bind all the symbols within
 * its loaded address space specified by image_start/image_end.
 * Make sure you understand why you need to check that each binding
 * must be performed within the image_start/image_end range.
 */
static int
bind_functions(struct Env *env, uint8_t *binary, size_t size, uintptr_t image_start, uintptr_t image_end) {
    struct Elf *elf_header = (struct Elf *)binary;

    COND_VERIFY(elf_header->e_shoff < size, E_INVALID_EXE);

    uint8_t *section_table = binary + elf_header->e_shoff;
    size_t sheader_count = elf_header->e_shnum;
    size_t sheader_size = elf_header->e_shentsize;

    COND_VERIFY(sheader_size >= sizeof(struct Secthdr), E_INVALID_EXE);

    size_t sh_table_size = sheader_count * sheader_size;
    size_t max_sh_offset = elf_header->e_shoff + sh_table_size;

    COND_VERIFY(sh_table_size / sheader_size == sheader_count, E_INVALID_EXE);
    COND_VERIFY(max_sh_offset > elf_header->e_shoff, E_INVALID_EXE);
    COND_VERIFY(max_sh_offset <= size, E_INVALID_EXE);

    char *strtab = NULL;
    size_t strtab_size = 0;

    uint8_t *symtab = NULL;
    size_t symtab_size = 0, symtab_entsize = 0;

    for (size_t sheader_offset = 0;
         sheader_offset < sheader_count * sheader_size;
         sheader_offset += sheader_size) {
        struct Secthdr *sheader = (struct Secthdr *)(section_table + sheader_offset);

        if (sheader->sh_type == ELF_SHT_SYMTAB) {
            symtab = binary + sheader->sh_offset;
            symtab_size = sheader->sh_size;
            symtab_entsize = sheader->sh_entsize;

            COND_VERIFY(sheader->sh_link < sheader_count, E_INVALID_EXE);

            size_t strtab_header_offset = sheader->sh_link * sheader_size;
            struct Secthdr *strtab_header = (struct Secthdr *)(section_table + strtab_header_offset);
            strtab = (char *)(binary + strtab_header->sh_offset);
            strtab_size = strtab_header->sh_size;

            const size_t max_symtab_offset = sheader->sh_offset + sheader->sh_size;
            const size_t max_strtab_offset = strtab_header->sh_offset + strtab_header->sh_size;

            COND_VERIFY(sheader->sh_offset <= max_symtab_offset, E_INVALID_EXE);
            COND_VERIFY(strtab_header->sh_offset <= max_strtab_offset, E_INVALID_EXE);
            COND_VERIFY(max_symtab_offset <= size, E_INVALID_EXE);
            COND_VERIFY(max_strtab_offset <= size, E_INVALID_EXE);

            break;
        }
    }

    if (!symtab_entsize)
        return -E_INVALID_EXE;

    /* NOTE: find_function from kdebug.c should be used */
    for (size_t sym_offset = 0;
         sym_offset < symtab_size;
         sym_offset += symtab_entsize) {
        struct Elf64_Sym *symbol = (struct Elf64_Sym *)(symtab + sym_offset);

        COND_VERIFY(symbol->st_name < strtab_size, E_INVALID_EXE);

        const char *symbol_name = strtab + symbol->st_name;

        int symbol_type = ELF64_ST_TYPE(symbol->st_info);
        int symbol_bind = ELF64_ST_BIND(symbol->st_info);

        if (symbol_type != STT_OBJECT ||
            symbol_bind != STB_GLOBAL) {
            continue;
        }

        if (image_start <= symbol->st_value && symbol->st_value < image_end) {
            uint8_t *var_addr = (uint8_t *)(symbol->st_value);
            uintptr_t func_addr = find_function(symbol_name);

            if (func_addr) {
                if (symbol->st_size != sizeof(func_addr))
                    return -E_INVALID_EXE;

                memcpy(var_addr, &func_addr, symbol->st_size);
            }
        }
    }

    return 0;
}

/**
 * Verify ELF header.
 * Return negative value if header is invalid;
 */
static int
verify_elf_header(struct Elf *header) {
    // Verify ELF magic
    if (header->e_magic != ELF_MAGIC)
        return -1;

    // TODO: Verify ELF class
    // TODO: Verify ELF data encoding
    // TODO: Verify ELF version
    // TODO: Verify ELF ABI

    // Verify ELF type
    if (header->e_type != ET_EXEC)
        return -1;

    // Verify ELF machine type
    if (header->e_machine != EM_X86_64)
        return -1;

    // TODO: Verify ELF file version
    return 0;
}

/* Set up the initial program binary, stack, and processor flags
 * for a user process.
 * This function is ONLY called during kernel initialization,
 * before running the first environment.
 *
 * This function loads all loadable segments from the ELF binary image
 * into the environment's user memory, starting at the appropriate
 * virtual addresses indicated in the ELF program header.
 * At the same time it clears to zero any portions of these segments
 * that are marked in the program header as being mapped
 * but not actually present in the ELF file - i.e., the program's bss section.
 *
 * All this is very similar to what our boot loader does, except the boot
 * loader also needs to read the code from disk.  Take a look at
 * LoaderPkg/Loader/Bootloader.c to get ideas.
 *
 * Finally, this function maps one page for the program's initial stack.
 *
 * load_icode returns -E_INVALID_EXE if it encounters problems.
 *  - How might load_icode fail?  What might be wrong with the given input?
 *
 * Hints:
 *   Load each program segment into memory
 *   at the address specified in the ELF section header.
 *   You should only load segments with ph->p_type == ELF_PROG_LOAD.
 *   Each segment's address can be found in ph->p_va
 *   and its size in memory can be found in ph->p_memsz.
 *   The ph->p_filesz bytes from the ELF binary, starting at
 *   'binary + ph->p_offset', should be copied to address
 *   ph->p_va.  Any remaining memory bytes should be cleared to zero.
 *   (The ELF header should have ph->p_filesz <= ph->p_memsz.)
 *
 *   All page protection bits should be user read/write for now.
 *   ELF segments are not necessarily page-aligned, but you can
 *   assume for this function that no two segments will touch
 *   the same page.
 *
 *   You must also do something with the program's entry point,
 *   to make sure that the environment starts executing there.
 *   What?  (See env_run() and env_pop_tf() below.) */
static int
load_icode(struct Env *env, uint8_t *binary, size_t size) {
    if (size < sizeof(struct Elf))
        return -E_INVALID_EXE;

    // Read elf header
    struct Elf *elf_header = (struct Elf *)binary;
    // Verify elf header
    COND_VERIFY(verify_elf_header(elf_header) == 0, E_INVALID_EXE);
    COND_VERIFY(elf_header->e_phoff < size, E_INVALID_EXE);

    uint8_t *pheader_table = binary + elf_header->e_phoff;
    size_t pheader_count = elf_header->e_phnum;
    size_t pheader_size = elf_header->e_phentsize;

    size_t ph_table_size = pheader_count * pheader_size;
    size_t max_ph_offset = elf_header->e_phoff + ph_table_size;

    COND_VERIFY(pheader_size >= sizeof(struct Proghdr), E_INVALID_EXE);
    COND_VERIFY(ph_table_size / pheader_size == pheader_count, E_INVALID_EXE);
    COND_VERIFY(max_ph_offset >= elf_header->e_phoff, E_INVALID_EXE);
    COND_VERIFY(max_ph_offset <= size, E_INVALID_EXE);

    size_t image_start = size;
    size_t image_end = 0;

    // Map segments to memory
    for (size_t pheader_off = 0;
         pheader_off < pheader_size * pheader_count;
         pheader_off += pheader_size) {
        struct Proghdr *pheader = (struct Proghdr *)(pheader_table + pheader_off);

        if (pheader->p_type != PT_LOAD) // Segment not mapped to memory
            continue;

        if (image_start > pheader->p_va) {
            image_start = pheader->p_va;
        }

        if (image_end < pheader->p_va + pheader->p_memsz) {
            image_end = pheader->p_va + pheader->p_memsz;
        }

        uint8_t *virt_addr = (uint8_t *)pheader->p_va;
        size_t seg_offset = pheader->p_offset;
        size_t mapped_size = pheader->p_memsz;
        size_t real_size = pheader->p_filesz;

        COND_VERIFY(seg_offset + real_size >= seg_offset, E_INVALID_EXE);
        COND_VERIFY(seg_offset + real_size <= size, E_INVALID_EXE);

        int map_res = map_region(
                &env->address_space,
                (uintptr_t)virt_addr,
                NULL, 0,
                mapped_size, PROT_RWX | PROT_USER_ | ALLOC_ZERO);
        if (map_res < 0) {
            return map_res;
        }

        int class = 0;
        while (CLASS_SIZE(class) < mapped_size +
                              (((uintptr_t)virt_addr) & CLASS_MASK(class))) {
          ++class;
        }

        int alloc_res = force_alloc_page(&env->address_space, 
                        (uintptr_t) virt_addr, class);
        if (alloc_res < 0) {
          return alloc_res;
        }

        struct AddressSpace *current_space = switch_address_space(
                &env->address_space);
        if (trace_envs_more) {
          cprintf("[%08x] Loading segment to %p (memsz=0x%zx, realsz=0x%zx)\n",
                  env->env_id, virt_addr, mapped_size, real_size);
        }
#ifdef SANITIZE_SHADOW_BASE
        // Unpoison mapped region
        platform_asan_unpoison(virt_addr, mapped_size);
#endif // SANITIZE_SHADOW_BASE
        memcpy(virt_addr, binary + seg_offset, real_size);

        switch_address_space(current_space);
    }

    env->binary = binary;
    env->env_tf.tf_rip = elf_header->e_entry;

#if 0 /* CONFIG_KSPACE */
    struct AddressSpace *current_space = switch_address_space(
            &env->address_space);

    int bind_result = bind_functions(env, binary, size, image_start, image_end);

    switch_address_space(current_space);

    return bind_result;
#else
    /* NOTE: When merging origin/lab10 put this hunk at the end
     *       of the function, when user stack is already mapped. */
    if (env->env_type == ENV_TYPE_FS) {
        /* If we are about to start filesystem server we need to pass
         * information about PCIe MMIO region to it. */
        struct AddressSpace *as = switch_address_space(&env->address_space);
        env->env_tf.tf_rsp = make_fs_args((char *)env->env_tf.tf_rsp);
        switch_address_space(as);
    }
    return 0;
#endif // CONFIG_KSPACE
}

/* Allocates a new env with env_alloc, loads the named elf
 * binary into it with load_icode, and sets its env_type.
 * This function is ONLY called during kernel initialization,
 * before running the first user-mode environment.
 * The new env's parent ID is set to 0.
 */
void
env_create(uint8_t *binary, size_t size, enum EnvType type) {
    struct Env *env = NULL;
    int status = 0;

    status = env_alloc(&env, 0, type);
    if (status < 0)
        panic("env alloc: %i", status);

    status = load_icode(env, binary, size);
    if (status < 0)
        panic("load icode: %i", status);
    // LAB 10: Your code here
}


/* Frees env and all memory it uses */
void
env_free(struct Env *env) {

    /* Note the environment's demise. */
    if (trace_envs) cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, env->env_id);

#ifndef CONFIG_KSPACE
    /* If freeing the current environment, switch to kern_pgdir
     * before freeing the page directory, just in case the page
     * gets reused. */
    if (&env->address_space == current_space)
        switch_address_space(&kspace);

    static_assert(MAX_USER_ADDRESS % HUGE_PAGE_SIZE == 0, "Misaligned MAX_USER_ADDRESS");
    release_address_space(&env->address_space);
#endif

    /* Return the environment to the free list */
    env->env_status = ENV_FREE;
    env->env_link = env_free_list;
    env_free_list = env;
}

/* Frees environment env
 *
 * If env was the current one, then runs a new environment
 * (and does not return to the caller)
 */
void
env_destroy(struct Env *env) {
    /* If env is currently running on other CPUs, we change its state to
     * ENV_DYING. A zombie environment will be freed the next time
     * it traps to the kernel. */

    // LAB 10: Your code here

    env_free(env);
    if (env == curenv) {
        /* Reset in_page_fault flags in case *current* environment
         * is getting destroyed after performing invalid memory access. */
        in_page_fault = false;
        sched_yield();
    }
}

#if 0 /* CONFIG_KSPACE */
void
csys_exit(void) {
    if (!curenv) panic("curenv = NULL");
    env_destroy(curenv);
}

void
csys_yield(struct Trapframe *tf) {
    memcpy(&curenv->env_tf, tf, sizeof(struct Trapframe));
    sched_yield();
}
#endif

/* Restores the register values in the Trapframe with the 'ret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

_Noreturn void
env_pop_tf(struct Trapframe *tf) {
    asm volatile(
            "movq %0, %%rsp\n"
            "movq 0(%%rsp), %%r15\n"
            "movq 8(%%rsp), %%r14\n"
            "movq 16(%%rsp), %%r13\n"
            "movq 24(%%rsp), %%r12\n"
            "movq 32(%%rsp), %%r11\n"
            "movq 40(%%rsp), %%r10\n"
            "movq 48(%%rsp), %%r9\n"
            "movq 56(%%rsp), %%r8\n"
            "movq 64(%%rsp), %%rsi\n"
            "movq 72(%%rsp), %%rdi\n"
            "movq 80(%%rsp), %%rbp\n"
            "movq 88(%%rsp), %%rdx\n"
            "movq 96(%%rsp), %%rcx\n"
            "movq 104(%%rsp), %%rbx\n"
            "movq 112(%%rsp), %%rax\n"
            "movw 120(%%rsp), %%es\n"
            "movw 128(%%rsp), %%ds\n"
            "addq $152,%%rsp\n" /* skip tf_trapno and tf_errcode */
            "iretq" ::"g"(tf)
            : "memory");

    /* Mostly to placate the compiler */
    panic("Reached unrecheble\n");
}

/* Context switch from curenv to env.
 * This function does not return.
 *
 * Step 1: If this is a context switch (a new environment is running):
 *       1. Set the current environment (if any) back to
 *          ENV_RUNNABLE if it is ENV_RUNNING (think about
 *          what other states it can be in),
 *       2. Set 'curenv' to the new environment,
 *       3. Set its status to ENV_RUNNING,
 *       4. Update its 'env_runs' counter,
 * Step 2: Use env_pop_tf() to restore the environment's
 *       registers and starting execution of process.

 * Hints:
 *    If this is the first call to env_run, curenv is NULL.
 *
 *    This function loads the new environment's state from
 *    env->env_tf.  Go back through the code you wrote above
 *    and make sure you have set the relevant parts of
 *    env->env_tf to sensible values.
 */
_Noreturn void
env_run(struct Env *env) {
    assert(env);

    if (trace_envs_more) {
        const char *state[] = {"FREE", "DYING", "RUNNABLE", "RUNNING", "NOT_RUNNABLE"};
        if (curenv) cprintf("[%08X] env stopped: %s\n", curenv->env_id, state[curenv->env_status]);
        cprintf("[%08X] env started: %s\n", env->env_id, state[env->env_status]);
    }

    if (curenv && curenv->env_status == ENV_RUNNING) { /* Context switch */
        curenv->env_status = ENV_RUNNABLE;
    }

    curenv = env;
    curenv->env_status = ENV_RUNNING;
    ++curenv->env_runs;

    switch_address_space(&env->address_space);
    env_pop_tf(&curenv->env_tf);
    // LAB 8: Your code here

    while (1);
}
