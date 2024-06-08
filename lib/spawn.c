#include "inc/error.h"
#include "inc/fs.h"
#include "inc/kmod/file.h"
#include "inc/types.h"
#include <inc/lib.h>
#include <inc/elf.h>
#include <inc/rpc.h>

/* Spawn a child process from a program image loaded from the file system.
 * prog: the pathname of the program to run.
 * argv: pointer to null-terminated array of pointers to strings,
 *   which will be passed to the child as its command-line arguments.
 * Returns child envid on success, < 0 on failure. */
int
spawn(const char *prog, const char **argv) {
    static union FiledRequest request;
    static envid_t filed = 0;

    if (!filed) {
        filed = kmod_find_any_version(FILED_MODNAME);
        assert(filed > 0);
    }

    if (strnlen(prog, MAXPATHLEN) == MAXPATHLEN) return -E_INVAL;
    strncpy(request.spawn.req_path, prog, MAXPATHLEN);

    size_t argc = 0;
    for (const char **arg = argv; *arg; ++arg) {
        ++argc;
    }
    request.spawn.req_argc = argc;

    size_t offset = 0;
    size_t arg_index = 0;
    size_t remaining_len = sizeof(request.spawn.req_strtab);
    for (const char **arg = argv; *arg; ++arg) {
        if (!remaining_len) return -E_INVAL;

        size_t len = strnlen(*arg, remaining_len);

        if (len == remaining_len) return -E_INVAL;
        request.spawn.req_argv[arg_index] = offset;
        strncpy(request.spawn.req_strtab + offset, *arg, len + 1);

        offset += len + 1;
        remaining_len -= len + 1;
        ++arg_index;
    }

    return rpc_execute(filed, FILED_REQ_SPAWN, &request, NULL);
}

/* Spawn, taking command-line arguments array directly on the stack.
 * NOTE: Must have a sentinal of NULL at the end of the args
 * (none of the args may be NULL). */
int
spawnl(const char *prog, const char *arg0, ...) {
    /* We calculate argc by advancing the args until we hit NULL.
     * The contract of the function guarantees that the last
     * argument will always be NULL, and that none of the other
     * arguments will be NULL. */
    int argc = 0;
    va_list vl;
    va_start(vl, arg0);
    while (va_arg(vl, void *) != NULL) argc++;
    va_end(vl);

    /* Now that we have the size of the args, do a second pass
     * and store the values in a VLA, which has the format of argv */
    const char *argv[argc + 2];
    argv[0] = arg0;
    argv[argc + 1] = NULL;

    va_start(vl, arg0);
    unsigned i;
    for (i = 0; i < argc; i++) {
        argv[i + 1] = va_arg(vl, const char *);
    }
    va_end(vl);

    return spawn(prog, argv);
}
