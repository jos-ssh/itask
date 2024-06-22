#include "inc/env.h"
#include "inc/mmu.h"
#include "inc/rpc.h"
#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>
#include <inc/kmod/file.h>

union Fsipc fsipcbuf __attribute__((aligned(PAGE_SIZE)));

union FiledRequest filed_req;
union FiledResponse filed_resp;

static int
filed_rpc_execute(unsigned type, union FiledRequest *req, union FiledResponse *resp) {
    static envid_t filed_env;

    if (!filed_env) {
        filed_env = kmod_find_any_version(FILED_MODNAME);
        int res = sys_unmap_region(CURENVID, &filed_resp, sizeof(filed_resp));
        assert(res == 0);
    }

    return rpc_execute(filed_env, type, &filed_req, (void **)&resp);
};

/* Send an inter-environment request to the file server, and wait for
 * a reply.  The request body should be in fsipcbuf, and parts of the
 * response may be written back to fsipcbuf.
 * type: request code, passed as the simple integer IPC value.
 * dstva: virtual address at which to receive reply page, 0 if none.
 * Returns result from the file server. */
static int
fsipc(unsigned type, void *dstva) {
    static envid_t fsenv;

    if (!fsenv) fsenv = ipc_find_env(ENV_TYPE_FS);

    static_assert(sizeof(fsipcbuf) == PAGE_SIZE, "Invalid fsipcbuf size");

    if (debug) {
        cprintf("[%08x] fsipc %d %08x\n",
                thisenv->env_id, type, *(uint32_t *)&fsipcbuf);
    }

    ipc_send(fsenv, type, &fsipcbuf, PAGE_SIZE, PROT_RW);
    size_t maxsz = PAGE_SIZE;
    return ipc_recv_from(fsenv, dstva, &maxsz, NULL);
}

static int devfile_flush(struct Fd *fd);
static ssize_t devfile_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfile_write(struct Fd *fd, const void *buf, size_t n);
static int devfile_stat(struct Fd *fd, struct Stat *stat);
static int devfile_trunc(struct Fd *fd, off_t newsize);

struct Dev devfile = {
        .dev_id = 'f',
        .dev_name = "file",
        .dev_read = devfile_read,
        .dev_close = devfile_flush,
        .dev_stat = devfile_stat,
        .dev_write = devfile_write,
        .dev_trunc = devfile_trunc};

/* Open a file (or directory).
 *
 * Returns:
 *  The file descriptor index on success
 *  -E_BAD_PATH if the path is too long (>= MAXPATHLEN)
 *  < 0 for other errors. */
int
open(const char *path, int flags, ...) {
    /* Find an unused file descriptor page using fd_alloc.
     * Then send a file-open request to the file server.
     * Include 'path' and 'omode' in request,
     * and map the returned file descriptor page
     * at the appropriate fd address.
     * FSREQ_OPEN returns 0 on success, < 0 on failure.
     *
     * (fd_alloc does not allocate a page, it just returns an
     * unused fd address.  Do you need to allocate a page?)
     *
     * Return the file descriptor index.
     * If any step after fd_alloc fails, use fd_close to free the
     * file descriptor. */

    int res;
    struct Fd *fd;

    if (strlen(path) >= MAXPATHLEN)
        return -E_BAD_PATH;


    if ((res = fd_alloc(&fd)) < 0) return res;

    strncpy(filed_req.open.req_path, path, MAXPATHLEN);
    filed_req.open.req_omode = flags;
    filed_req.open.req_fd_vaddr = (uintptr_t)fd;

    if ((res = filed_rpc_execute(FILED_REQ_OPEN, &filed_req, NULL)) < 0) {
        fd_close(fd, 0);
        return res;
    }

    return fd2num(fd);
}

int 
get_cwd(char* buffer) {
    int res = 0;
    res = filed_rpc_execute(FILED_REQ_GETCWD, &filed_req, &filed_resp); /* filed_req isn`t really necessary here */
    
    if (res < 0) return res;
    strlcpy(buffer, filed_resp.cwd, MAXPATHLEN);
    
    return 0;
}

int set_cwd(const char* path) {
    strlcpy(filed_req.setcwd.req_path, path, MAXPATHLEN);
    return filed_rpc_execute(FILED_REQ_SETCWD, &filed_req, NULL);
}

int 
chmod(const char* path, uint32_t mode) {
    filed_req.chmod.req_mode = mode;
    strlcpy(filed_req.chmod.req_path, path, MAXPATHLEN);
    
    return filed_rpc_execute(FILED_REQ_CHMOD, &filed_req, NULL);
}

int
open_raw_fs(const char *path, int flags, ...) {
    /* Find an unused file descriptor page using fd_alloc.
     * Then send a file-open request to the file server.
     * Include 'path' and 'omode' in request,
     * and map the returned file descriptor page
     * at the appropriate fd address.
     * FSREQ_OPEN returns 0 on success, < 0 on failure.
     *
     * (fd_alloc does not allocate a page, it just returns an
     * unused fd address.  Do you need to allocate a page?)
     *
     * Return the file descriptor index.
     * If any step after fd_alloc fails, use fd_close to free the
     * file descriptor. */

    if (thisenv->env_type != ENV_TYPE_KERNEL) {
        return -E_BAD_ENV;
    }
    int res;
    struct Fd *fd;

    if (strlen(path) >= MAXPATHLEN)
        return -E_BAD_PATH;


    if ((res = fd_alloc(&fd)) < 0) return res;


    strcpy(fsipcbuf.open.req_path, path);
    fsipcbuf.open.req_oflags = flags;
    fsipcbuf.open.req_gid = 0;
    fsipcbuf.open.req_uid = 0;

    if (flags & O_CREAT) {
        va_list mode;
        va_start(mode, flags);

        fsipcbuf.open.req_omode = va_arg(mode, uint32_t);

        va_end(mode);
    }

    if ((res = fsipc(FSREQ_OPEN, fd)) < 0) {
        fd_close(fd, 0);
        return res;
    }

    return fd2num(fd);
}

/* Flush the file descriptor.  After this the fileid is invalid.
 *
 * This function is called by fd_close.  fd_close will take care of
 * unmapping the FD page from this environment.  Since the server uses
 * the reference counts on the FD pages to detect which files are
 * open, unmapping it is enough to free up server-side resources.
 * Other than that, we just have to make sure our changes are flushed
 * to disk. */
static int
devfile_flush(struct Fd *fd) {
    fsipcbuf.flush.req_fileid = fd->fd_file.id;
    return fsipc(FSREQ_FLUSH, NULL);
}

/* Read at most 'n' bytes from 'fd' at the current position into 'buf'.
 *
 * Returns:
 *  The number of bytes successfully read.
 *  < 0 on error. */
static ssize_t
devfile_read(struct Fd *fd, void *buf, size_t n) {
    /* Make an FSREQ_READ request to the file system server after
     * filling fsipcbuf.read with the request arguments.  The
     * bytes read will be written back to fsipcbuf by the file
     * system server. */

    size_t remaining = n;
    while (remaining > 0) {
        fsipcbuf.read.req_fileid = fd->fd_file.id;
        fsipcbuf.read.req_n = MIN(remaining, sizeof(fsipcbuf.readRet.ret_buf));
        int res = fsipc(FSREQ_READ, &fsipcbuf);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        memcpy(buf + n - remaining, fsipcbuf.readRet.ret_buf, res);
        remaining -= res;
    }

    return n - remaining;
}

/* Write at most 'n' bytes from 'buf' to 'fd' at the current seek position.
 *
 * Returns:
 *   The number of bytes successfully written.
 *   < 0 on error. */
static ssize_t
devfile_write(struct Fd *fd, const void *buf, size_t n) {
    /* Make an FSREQ_WRITE request to the file system server.  Be
     * careful: fsipcbuf.write.req_buf is only so large, but
     * remember that write is always allowed to write *fewer*
     * bytes than requested, so that multiple IPC requests are
     * potentially required. */

    // LAB 10: Your code here:
    size_t remaining = n;
    while (remaining > 0) {
        const size_t bufsize = sizeof(fsipcbuf.write.req_buf);
        fsipcbuf.write.req_fileid = fd->fd_file.id;
        fsipcbuf.write.req_n = MIN(remaining, bufsize);
        memcpy(fsipcbuf.write.req_buf, buf + n - remaining, MIN(bufsize, remaining));

        int res = fsipc(FSREQ_WRITE, &fsipcbuf);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        remaining -= res;
    }

    return n - remaining;
}

/* Get file information */
static int
devfile_stat(struct Fd *fd, struct Stat *st) {
    fsipcbuf.stat.req_fileid = fd->fd_file.id;
    int res = fsipc(FSREQ_STAT, NULL);
    if (res < 0) return res;

    strcpy(st->st_name, fsipcbuf.statRet.ret_name);
    st->st_size = fsipcbuf.statRet.ret_size;
    st->st_isdir = fsipcbuf.statRet.ret_isdir;
    st->st_mode = fsipcbuf.statRet.ret_mode;
    st->st_gid = fsipcbuf.statRet.ret_gid;
    st->st_uid = fsipcbuf.statRet.ret_uid;

    return 0;
}

/* Truncate or extend an open file to 'size' bytes */
static int
devfile_trunc(struct Fd *fd, off_t newsize) {
    fsipcbuf.set_size.req_fileid = fd->fd_file.id;
    fsipcbuf.set_size.req_size = newsize;

    return fsipc(FSREQ_SET_SIZE, NULL);
}

/* Synchronize disk with buffer cache */
int
sync(void) {
    /* Ask the file server to update the disk
     * by writing any dirty blocks in the buffer cache. */

    return fsipc(FSREQ_SYNC, NULL);
}

int 
remove(const char *path) {
    if (strlen(path) >= MAXPATHLEN)
        return -E_BAD_PATH;

    strlcpy(fsipcbuf.remove.req_path, path, MAXPATHLEN);
    return fsipc(FSREQ_REMOVE, &fsipcbuf);
}

int 
mkdir(const char *path, int mode) {
    if (strlen(path) >= MAXPATHLEN)
        return -E_BAD_PATH;

    fsipcbuf.mkdir.req_gid = 0;
    fsipcbuf.mkdir.req_uid = 0;
    fsipcbuf.mkdir.req_omode = mode;
    strlcpy(fsipcbuf.mkdir.req_path, path, MAXPATHLEN);
    return fsipc(FSREQ_MKDIR, &fsipcbuf);
}

int
getdents(const char* path, struct FileInfo* buffer, uint32_t count) {
    strlcpy(fsipcbuf.getdents.req_path, path, MAXPATHLEN);

    int remaining = count;
    while (remaining > 0)
    {
        const uint32_t getdents_count = MIN(remaining, MAX_GETDENTS_COUNT); 
        fsipcbuf.getdents.count = getdents_count;
        fsipcbuf.getdents.from_which_count = remaining - getdents_count;
        int res = fsipc(FSREQ_GETDENTS, &fsipcbuf);
        if (res < 0)
            return res;
        
        memcpy(buffer + remaining - getdents_count, fsipcbuf.getdents.buffer, sizeof(struct FileInfo) * getdents_count);
        remaining -= getdents_count;
    }

    return 0;
}
