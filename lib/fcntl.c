#include "inc/error.h"
#include "inc/fd.h"
#include "inc/stdarg.h"
#include "inc/unistd.h"
#include <inc/fcntl.h>
#include <inc/lib.h>

int
fcntl(int fdnum, int cmd, ... /* arg */) { /* NOTIMPLEMENTED(int); */
    struct Fd* fd;
    int res = fd_lookup(fdnum, &fd);
    if (res < 0)
        return res;

    if (cmd == F_GETFL) {
        return fd->fd_omode;
    }

    if (cmd == F_SETFL) {
        va_list args;
        va_start(args, cmd);
        int arg = va_arg(args, int);
        va_end(args);

        int new_flags = fd->fd_omode & ~O_MOD_FLAGS;
        new_flags |= arg & O_MOD_FLAGS;
        fd->fd_omode = new_flags;

        return new_flags;
    }

    return -E_NOT_SUPP;
}
