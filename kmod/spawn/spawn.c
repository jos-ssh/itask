#include "spawn.h"

#include "inc/env.h"
#include "inc/fd.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/stdio.h"
#include "inc/types.h"
#include <inc/lib.h>
#include <inc/elf.h>

envid_t
spawnd_fork(envid_t parent) {
    // TODO: implement
    void *parent_upcall = thisenv->env_pgfault_upcall;
    envid_t child = sys_exofork();

    if (child < 0)
        return child;

    if (child == 0) {
        if (parent_upcall)
            sys_env_set_pgfault_upcall(CURENVID, parent_upcall);

        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    sys_map_region(parent, NULL, child, NULL,
                   MAX_USER_ADDRESS, PROT_ALL | PROT_LAZY | PROT_COMBINE);

    int status_res = sys_env_set_status(child, ENV_RUNNABLE);
    if (status_res < 0) {
        return status_res;
    }

    return child;
}

#define UTEMP2USTACK(addr) ((void *)(addr) + (USER_STACK_TOP - USER_STACK_SIZE) - UTEMP)

/* Helper functions for spawn and exec. */
static int load_executable(envid_t child, int fd, const char **argv);

static int init_stack(envid_t child, const char **argv, struct Trapframe *tf);
static int map_segment(envid_t child, uintptr_t va, size_t memsz,
                       int fd, size_t filesz, off_t fileoffset, int perm);
static int copy_shared_region(void *start, void *end, void *arg);

/* Spawn a child process from a program image loaded from the file system.
 * prog: the pathname of the program to run.
 * argv: pointer to null-terminated array of pointers to strings,
 *   which will be passed to the child as its command-line arguments.
 * Returns child envid on success, < 0 on failure. */
int
spawnd_spawn(envid_t parent, const char *prog, const char **argv) {
    cprintf("spawnd_spawn(parent=%08x, prog=%s, argv=[", parent, prog);
    const char** argp = argv;
    while (*argp)
    {
      cprintf("\"%s\"", *argp);
      ++argp;
      if (*argp) {
        cprintf(", ");
      }
    }
    cprintf("])\n");


    int res;

    /* This code follows this procedure:
     *
     *   - Open the program file.
     *   - TODO: check file is executable
     *   - Use sys_exofork() to create a new environment.
     *   - Load child with code from file
     *   - Copy open file descriptors
     *   - Start the child process running with sys_env_set_status(). */

    int fd = open(prog, O_RDONLY);
    if (fd < 0) return fd;

    /* Create new child environment */
    // TODO: set parent appropriately
    if ((int)(res = sys_exofork()) < 0) goto error2;
    envid_t child = res;

    if ((int)(res = load_executable(child, fd, argv)) < 0) goto error;
    close(fd);

    /* TODO: copy file descriptors separately to handle possible O_CLOEXEC */
    /* Copy file descriptors. */
    res = sys_map_region(parent, (void *)FDTABLE, child, (void *)FDTABLE,
                   2 * MAXFD * PAGE_SIZE, PROT_ALL | PROT_SHARE | PROT_COMBINE);
   
    if (res < 0) {
      panic("FD copy: %i\n", res);
    }

    if ((res = sys_env_set_status(child, ENV_RUNNABLE)) < 0)
        panic("sys_env_set_status: %i", res);

    return child;

error:
    sys_env_destroy(child);
error2:
    close(fd);

    return res;
}

static int
load_executable(envid_t child, int fd, const char **argv) {
    unsigned char elf_buf[512];
    int res;
    /*
     *   - Read the ELF header, as you have before, and sanity check its
     *     magic number.  (Check out your load_icode!)
     *
     *   - Set child_tf to an initial struct Trapframe for the child.
     *
     *   - Call the init_stack() function above to set up
     *     the initial stack page for the child environment.
     *
     *   - Map all of the program's segments that are of p_type
     *     ELF_PROG_LOAD into the new environment's address space.
     *     Use the p_flags field in the Proghdr for each segment
     *     to determine how to map the segment:
     *
     *    * If the ELF flags do not include ELF_PROG_FLAG_WRITE,
     *      then the segment contains text and read-only data.
     *      Use read_map() to read the contents of this segment,
     *      and map the pages it returns directly into the child
     *        so that multiple instances of the same program
     *      will share the same copy of the program text.
     *        Be sure to map the program text read-only in the child.
     *        Read_map is like read but returns a pointer to the data in
     *        *blk rather than copying the data into another buffer.
     *
     *    * If the ELF segment flags DO include ELF_PROG_FLAG_WRITE,
     *      then the segment contains read/write data and bss.
     *      As with load_icode() in Lab 3, such an ELF segment
     *      occupies p_memsz bytes in memory, but only the FIRST
     *      p_filesz bytes of the segment are actually loaded
     *      from the executable file - you must clear the rest to zero.
     *        For each page to be mapped for a read/write segment,
     *        allocate a page in the parent temporarily at UTEMP,
     *        read() the appropriate portion of the file into that page
     *      and/or use memset() to zero non-loaded portions.
     *      (You can avoid calling memset(), if you like, if
     *      page_alloc() returns zeroed pages already.)
     *        Then insert the page mapping into the child.
     *        Look at init_stack() for inspiration.
     *        Be sure you understand why you can't use read_map() here.
     *
     *     Note: None of the segment addresses or lengths above
     *     are guaranteed to be page-aligned, so you must deal with
     *     these non-page-aligned values appropriately.
     *     The ELF linker does, however, guarantee that no two segments
     *     will overlap on the same page; and it guarantees that
     *     PGOFF(ph->p_offset) == PGOFF(ph->p_va).
     *
     *   - Call sys_env_set_trapframe(child, &child_tf) to set up the
     *     correct initial eip and esp values in the child.
     *
     */

    /* Read elf header */
    struct Elf *elf = (struct Elf *)elf_buf;
    res = readn(fd, elf_buf, sizeof(elf_buf));
    if (res != sizeof(elf_buf)) {
        cprintf("Wrong ELF header size or read error: %i\n", res);
        close(fd);
        return -E_NOT_EXEC;
    }
    if (elf->e_magic != ELF_MAGIC ||
        elf->e_elf[0] != 2 /* 64-bit */ ||
        elf->e_elf[1] != 1 /* little endian */ ||
        elf->e_elf[2] != 1 /* version 1 */ ||
        elf->e_type != ET_EXEC /* executable */ ||
        elf->e_machine != 0x3E /* amd64 */) {
        cprintf("Elf magic %08x want %08x\n", elf->e_magic, ELF_MAGIC);
        close(fd);
        return -E_NOT_EXEC;
    }

    /* Set up trap frame, including initial stack. */
    struct Trapframe child_tf = envs[ENVX(child)].env_tf;
    child_tf.tf_rip = elf->e_entry;

    if ((res = init_stack(child, argv, &child_tf)) < 0) goto error;

    /* Set up program segments as defined in ELF header. */
    struct Proghdr *ph = (struct Proghdr *)(elf_buf + elf->e_phoff);
    for (size_t i = 0; i < elf->e_phnum; i++, ph++) {
        if (ph->p_type != ELF_PROG_LOAD) continue;
        int perm = 0;

        if (ph->p_flags & ELF_PROG_FLAG_WRITE) perm |= PROT_W;
        if (ph->p_flags & ELF_PROG_FLAG_READ) perm |= PROT_R;
        if (ph->p_flags & ELF_PROG_FLAG_EXEC) perm |= PROT_X;

        if ((res = map_segment(child, ph->p_va, ph->p_memsz,
                               fd, ph->p_filesz, ph->p_offset, perm)) < 0)
            goto error;
    }

    if ((res = sys_env_set_trapframe(child, &child_tf)) < 0)
        panic("sys_env_set_trapframe: %i", res);

error:
    close(fd);

    return res;
}

/* Set up the initial stack page for the new child process with envid 'child'
 * using the arguments array pointed to by 'argv',
 * which is a null-terminated array of pointers to null-terminated strings.
 *
 * On success, returns 0 and sets *init_esp
 * to the initial stack pointer with which the child should start.
 * Returns < 0 on failure. */
static int
init_stack(envid_t child, const char **argv, struct Trapframe *tf) {
    size_t string_size;
    int argc, i, res;
    char *string_store;
    uintptr_t *argv_store;

    /* Count the number of arguments (argc)
     * and the total amount of space needed for strings (string_size). */
    string_size = 0;
    for (argc = 0; argv[argc] != 0; argc++)
        string_size += strlen(argv[argc]) + 1;

    /* Determine where to place the strings and the argv array.
     * Set up pointers into the temporary page 'UTEMP'; we'll map a page
     * there later, then remap that page into the child environment
     * at (USER_STACK_TOP - USER_STACK_SIZE).
     * strings is the topmost thing on the stack. */
    string_store = (char *)UTEMP + USER_STACK_SIZE - string_size;
    /* argv is below that.  There's one argument pointer per argument, plus
     * a null pointer. */
    argv_store = (uintptr_t *)(ROUNDDOWN(string_store, sizeof(uintptr_t)) - sizeof(uintptr_t) * (argc + 1));

    /* Make sure that argv, strings, and the 2 words that hold 'argc'
     * and 'argv' themselves will all fit in a single stack page. */
    if ((void *)(argv_store - 2) < (void *)UTEMP) return -E_NO_MEM;

    /* Allocate the stack pages at UTEMP. */
    if ((res = sys_alloc_region(0, UTEMP, USER_STACK_SIZE, PROT_RW)) < 0) return res;

    /*    * Initialize 'argv_store[i]' to point to argument string i,
     *      for all 0 <= i < argc.
     *      Also, copy the argument strings from 'argv' into the
     *      newly-allocated stack page.
     *
     *    * Set 'argv_store[argc]' to 0 to null-terminate the args array.
     *
     *    * Push two more words onto the child's stack below 'args',
     *      containing the argc and argv parameters to be passed
     *      to the child's umain() function.
     *      argv should be below argc on the stack.
     *      (Again, argv should use an address valid in the child's
     *      environment.)
     *
     *    * Set *init_esp to the initial stack pointer for the child,
     *      (Again, use an address valid in the child's environment.) */
    for (i = 0; i < argc; i++) {
        argv_store[i] = UTEMP2USTACK(string_store);
        strcpy(string_store, argv[i]);
        string_store += strlen(argv[i]) + 1;
    }
    argv_store[argc] = 0;
    assert(string_store == (char *)UTEMP + USER_STACK_SIZE);

    argv_store[-1] = UTEMP2USTACK(argv_store);
    argv_store[-2] = argc;


    tf->tf_rsp = UTEMP2USTACK(&argv_store[-2]);

    /* After completing the stack, map it into the child's address space
     * and unmap it from ours! */
    if (sys_map_region(0, UTEMP, child, (void *)(USER_STACK_TOP - USER_STACK_SIZE),
                       USER_STACK_SIZE, PROT_RW) < 0) goto error;
error:
    if (sys_unmap_region(0, UTEMP, USER_STACK_SIZE) < 0) goto error;
    return res;
}

static int
copy_shared_region(void *start, void *end, void *arg) {
    envid_t child = *(envid_t *)arg;
    return sys_map_region(0, start, child, start, end - start, get_prot(start));
}


static int
map_segment(envid_t child, uintptr_t va, size_t memsz,
            int fd, size_t filesz, off_t fileoffset, int perm) {

    // cprintf("map_segment %x+%x\n", va, memsz);

    /* Fixup unaligned destination */
    int offset = PAGE_OFFSET(va);
    if (offset) {
        va -= offset;
        memsz += offset;
        filesz += offset;
        fileoffset -= offset;
    }

    // LAB 11: Your code here
    /* NOTE: There's restriction on maximal filesz
     * for each program segment (HUGE_PAGE_SIZE) */
    if (filesz > HUGE_PAGE_SIZE) {
        return -E_INVAL;
    }

    int res = 0;
    void *const temp_mem = (void *)UTEMP;
    const size_t temp_size = ROUNDUP(filesz, PAGE_SIZE);

    if (filesz > 0) {
        /* Allocate filesz in parent to UTEMP */
        res = sys_alloc_region(CURENVID, temp_mem, temp_size, PROT_RW | perm);
        assert(res >= 0);
        if (res < 0) goto end;

        /* seek() fd to fileoffset  */
        res = seek(fd, fileoffset);
        assert(res >= 0);
        if (res < 0) goto unmap_temp;

        /* read filesz to UTEMP */
        ssize_t count = read(fd, temp_mem, filesz);
        if (count < 0) {
            res = count;
            assert(res >= 0);
            goto unmap_temp;
        }
        /* Map read section conents to child */
        res = sys_map_region(CURENVID, temp_mem, child, (void *)va, temp_size, perm);
        assert(res >= 0);
        if (res < 0) goto unmap_temp;

    unmap_temp:
        /* Unmap temporary memory from parent */
        {
            int unmap_res = sys_unmap_region(CURENVID, temp_mem, temp_size);
            if (unmap_res < 0) {
                return unmap_res;
            }
        }
        if (res < 0) goto end;
    }

    /* Allocate filesz - memsz in child */
    if (memsz > temp_size) {
        const uintptr_t alloc_start = va + temp_size;
        const uintptr_t alloc_size = ROUNDUP(memsz, PAGE_SIZE) - temp_size;
        res = sys_alloc_region(child, (void *)alloc_start, alloc_size, perm);
        assert(res >= 0);
        if (res < 0) goto end;
    }

end:
    assert(res >= 0);
    return res;
}
