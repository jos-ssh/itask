/*
 * File system server main loop -
 * serves IPC requests from other environments.
 */

#include <inc/x86.h>
#include <inc/string.h>

#include "inc/mmu.h"
#include "inc/stdio.h"
#include "pci.h"
#include "fs.h"
#include "nvme.h"

/* The file system server maintains three structures
 * for each open file.
 *
 * 1. The on-disk 'struct File' is mapped into the part of memory
 *    that maps the disk.  This memory is kept private to the file
 *    server.
 * 2. Each open file has a 'struct Fd' as well, which sort of
 *    corresponds to a Unix file descriptor.  This 'struct Fd' is kept
 *    on *its own page* in memory, and it is shared with any
 *    environments that have the file open.
 * 3. 'struct OpenFile' links these other two structures, and is kept
 *    private to the file server.  The server maintains an array of
 *    all open files, indexed by "file ID".  (There can be at most
 *    MAXOPEN files open concurrently.)  The client uses file IDs to
 *    communicate with the server.  File IDs are a lot like
 *    environment IDs in the kernel.  Use openfile_lookup to translate
 *    file IDs to struct OpenFile. */

struct OpenFile {
    uint32_t o_fileid;   /* file id */
    struct File *o_file; /* mapped descriptor for open file */
    int o_mode;          /* open mode */
    struct Fd *o_fd;     /* Fd page */
};

/* initialize to force into data section */
struct OpenFile opentab[MAXOPEN] = {
        {0, 0, 1, 0}};

/* Virtual address at which to receive page mappings containing client requests. */
union Fsipc *fsreq = (union Fsipc *)0x0FFFF000;

void
serve_init(void) {
    uintptr_t va = FILE_BASE;
    for (size_t i = 0; i < MAXOPEN; i++) {
        opentab[i].o_fileid = i;
        opentab[i].o_fd = (struct Fd *)va;
        va += PAGE_SIZE;
    }
}

/* Allocate an open file. */
int
openfile_alloc(struct OpenFile **o) {

    /* Find an available open-file table entry */
    for (size_t i = 0; i < MAXOPEN; i++) {
        switch (sys_region_refs(opentab[i].o_fd, PAGE_SIZE)) {
        case 0: {
            int res = sys_alloc_region(0, opentab[i].o_fd, PAGE_SIZE, PROT_RW);
            if (res < 0) return res;
        }
        /* fallthrough */
        case 1:
            opentab[i].o_fileid += MAXOPEN;
            *o = &opentab[i];
            memset(opentab[i].o_fd, 0, PAGE_SIZE);
            return (*o)->o_fileid;
        }
    }
    return -E_MAX_OPEN;
}

/* Look up an open file for envid. */
int
openfile_lookup(envid_t envid, uint32_t fileid, struct OpenFile **po) {
    struct OpenFile *o;

    o = &opentab[fileid % MAXOPEN];
    if (sys_region_refs(o->o_fd, PAGE_SIZE) <= 1 || o->o_fileid != fileid)
        return -E_INVAL;
    *po = o;
    return 0;
}

/* Open req->req_path in mode req->req_omode, storing the Fd page and
 * permissions to return to the calling environment in *pg_store and
 * *perm_store respectively. */
int
serve_open(envid_t envid, struct Fsreq_open *req,
           void **pg_store, int *perm_store) {
    char path[MAXPATHLEN];
    struct File *f;
    int res;
    struct OpenFile *o;

    if (debug) {
        cprintf("serve_open %08x %s 0x%x\n", envid, req->req_path, req->req_oflags);
    }

    /* Copy in the path, making sure it's null-terminated */
    memmove(path, req->req_path, MAXPATHLEN);

    path[MAXPATHLEN - 1] = 0;

    /* Find an open file ID */
    if ((res = openfile_alloc(&o)) < 0) {
        if (debug) cprintf("openfile_alloc failed: %i", res);
        return res;
    }

    /* Open the file */
    if (req->req_oflags & O_CREAT) {
        if ((res = file_create(path, &f, req->req_omode, req->req_gid, req->req_uid)) < 0) {
            if (!(req->req_oflags & O_EXCL) && res == -E_FILE_EXISTS)
                goto try_open;
            if (debug) cprintf("file_create failed: %i", res);
            return res;
        }
    } else {
    try_open:
        if ((res = file_open(path, &f)) < 0) {
            if (debug) cprintf("file_open failed: %i", res);
            return res;
        }
    }

    /* Truncate */
    if (req->req_oflags & O_TRUNC) {
        if ((res = file_set_size(f, 0)) < 0) {
            if (debug) cprintf("file_set_size failed: %i", res);
            return res;
        }
    }
    if ((res = file_open(path, &f)) < 0) {
        if (debug) cprintf("file_open failed: %i", res);
        return res;
    }

    /* Save the file pointer */
    o->o_file = f;

    /* Fill out the Fd structure */
    o->o_fd->fd_file.id = o->o_fileid;
    o->o_fd->fd_omode = req->req_oflags & O_ACCMODE;
    o->o_fd->fd_dev_id = devfile.dev_id;
    o->o_mode = req->req_oflags;

    if (debug) cprintf("sending success, page %08lx\n", (unsigned long)o->o_fd);

    /* Share the FD page with the caller by setting *pg_store,
     * store its permission in *perm_store */
    *pg_store = o->o_fd;
    *perm_store = PROT_RW | PROT_SHARE;

    return 0;
}

/* Set the size of req->req_fileid to req->req_size bytes, truncating
 * or extending the file as necessary. */
int
serve_set_size(envid_t envid, union Fsipc *ipc) {
    struct Fsreq_set_size *req = &ipc->set_size;
    struct OpenFile *o;
    int r;

    if (debug) {
        cprintf("serve_set_size %08x %08x %08x\n",
                envid, req->req_fileid, req->req_size);
    }

    /* Every file system IPC call has the same general structure.
     * Here's how it goes. */

    /* First, use openfile_lookup to find the relevant open file.
     * On failure, return the error code to the client with ipc_send. */
    if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
        return r;

    /* Second, call the relevant file system function (from fs/fs.c).
     * On failure, return the error code to the client. */
    return file_set_size(o->o_file, req->req_size);
}

/* Read at most ipc->read.req_n bytes from the current seek position
 * in ipc->read.req_fileid.  Return the bytes read from the file to
 * the caller in ipc->readRet, then update the seek position.  Returns
 * the number of bytes successfully read, or < 0 on error. */
int
serve_read(envid_t envid, union Fsipc *ipc) {
    struct Fsreq_read *req = &ipc->read;

    if (debug) {
        cprintf("serve_read %08x %08x %08x\n",
                envid, req->req_fileid, (uint32_t)req->req_n);
    }

    struct OpenFile *file = NULL;
    int lookup_res = openfile_lookup(envid, req->req_fileid, &file);
    if (lookup_res < 0) {
        return lookup_res;
    }

    const size_t bufsize = sizeof(ipc->readRet.ret_buf);
    int read_res = file_read(file->o_file, ipc->readRet.ret_buf,
                             MIN(req->req_n, bufsize), file->o_fd->fd_offset);
    if (read_res > 0) {
        file->o_fd->fd_offset += read_res;
    }
    return read_res;
}

/* Write req->req_n bytes from req->req_buf to req_fileid, starting at
 * the current seek position, and update the seek position
 * accordingly.  Extend the file if necessary.  Returns the number of
 * bytes written, or < 0 on error. */
int
serve_write(envid_t envid, union Fsipc *ipc) {
    struct Fsreq_write *req = &ipc->write;
    if (debug)
        cprintf("serve_write %08x %08x %08x\n", envid, req->req_fileid, (uint32_t)req->req_n);

    struct OpenFile *file = NULL;
    int lookup_res = openfile_lookup(envid, req->req_fileid, &file);
    if (lookup_res < 0) {
        return lookup_res;
    }

    const size_t bufsize = sizeof(req->req_buf);
    int write_res = file_write(file->o_file, req->req_buf,
                               MIN(req->req_n, bufsize), file->o_fd->fd_offset);
    if (write_res > 0) {
        file->o_fd->fd_offset += write_res;
    }

    return write_res;
}

/* Stat ipc->stat.req_fileid.  Return the file's struct Stat to the
 * caller in ipc->statRet. */
int
serve_stat(envid_t envid, union Fsipc *ipc) {
    struct Fsreq_stat *req = &ipc->stat;
    struct Fsret_stat *ret = &ipc->statRet;

    if (debug) cprintf("serve_stat %08x %08x\n", envid, req->req_fileid);

    struct OpenFile *o;
    int res = openfile_lookup(envid, req->req_fileid, &o);
    if (res < 0) return res;

    strcpy(ret->ret_name, o->o_file->f_name);
    ret->ret_size = o->o_file->f_size;
    ret->ret_isdir = (ISDIR(o->o_file->f_mode));
    ret->ret_uid = o->o_file->f_uid;
    ret->ret_gid = o->o_file->f_gid;
    ret->ret_mode = o->o_file->f_mode;

    return 0;
}

/* Flush all data and metadata of req->req_fileid to disk. */
int
serve_flush(envid_t envid, union Fsipc *ipc) {
    struct Fsreq_flush *req = &ipc->flush;
    if (debug) cprintf("serve_flush %08x %08x\n", envid, req->req_fileid);

    struct OpenFile *o;
    int res = openfile_lookup(envid, req->req_fileid, &o);
    if (res < 0) return res;

    file_flush(o->o_file);
    return 0;
}

int
serve_sync(envid_t envid, union Fsipc *req) {
    fs_sync();
    return 0;
}

int
serve_remove(envid_t envid, union Fsipc *req) {
    return file_remove(req->remove.req_path);
}

int
serve_mkdir(envid_t envid, union Fsipc *req) {
    struct File *f = NULL;
    return file_create(req->mkdir.req_path, &f, IFDIR | req->mkdir.req_omode, req->mkdir.req_gid, req->mkdir.req_uid);
}

int
serve_chmod(envid_t envid, union Fsipc *req) {
    return file_chmod(req->chmod.req_path, req->chmod.req_mode);
}

int
serve_chown(envid_t envid, union Fsipc *req) {
    return file_chown(req->chown.req_path, req->chown.req_gid, req->chown.req_uid);
}

int
serve_getdents(envid_t envid, union Fsipc *req, void **page) {
    *page = (void *)req;
    return file_getdents(req->getdents.req_path, req->getdents.buffer, req->getdents.count, req->getdents.from_which_count);
}

typedef int (*fshandler)(envid_t envid, union Fsipc *req);

fshandler handlers[] = {
        /* Open is handled specially because it passes pages */
        //[FSREQ_OPEN] =   (fshandler)serve_open,
        [FSREQ_READ] = serve_read,
        [FSREQ_STAT] = serve_stat,
        [FSREQ_FLUSH] = serve_flush,
        [FSREQ_WRITE] = serve_write,
        [FSREQ_SET_SIZE] = serve_set_size,
        [FSREQ_SYNC] = serve_sync,
        [FSREQ_REMOVE] = serve_remove,
        [FSREQ_MKDIR] = serve_mkdir,
        [FSREQ_CHMOD] = serve_chmod,
        [FSREQ_CHOWN] = serve_chown};
#define NHANDLERS (sizeof(handlers) / sizeof(handlers[0]))

void
serve(void) {
    uint32_t req, whom;
    int perm, res;
    void *pg;

    while (1) {
        perm = 0;
        size_t sz = PAGE_SIZE;
        req = ipc_recv((int32_t *)&whom, fsreq, &sz, &perm);
        if (debug) {
            cprintf("fs req %d from %08x [page %08lx: %s]\n",
                    req, whom, (unsigned long)get_uvpt_entry(fsreq),
                    (char *)fsreq);
        }

        /* All requests must contain an argument page */
        if (!(perm & PROT_R)) {
            cprintf("Invalid request from %08x: no argument page\n", whom);
            continue; /* Just leave it hanging... */
        }

        pg = NULL;
        if (req == FSREQ_OPEN) {
            res = serve_open(whom, (struct Fsreq_open *)fsreq, &pg, &perm);
        } else if (req == FSREQ_GETDENTS) {
            res = serve_getdents(whom, fsreq, &pg);
        } else if (req < NHANDLERS && handlers[req]) {
            res = handlers[req](whom, fsreq);
        } else {
            cprintf("Invalid request code %d from %08x\n", req, whom);
            res = -E_INVAL;
        }
        ipc_send(whom, res, pg, PAGE_SIZE, perm);
        sys_unmap_region(0, fsreq, PAGE_SIZE);
    }
}

void
umain(int argc, char **argv) {
    static_assert(sizeof(struct File) == 256, "Unsupported file size");
    binaryname = "fs";
    cprintf("FS is running\n");

    pci_init(argv);
    nvme_init();

    /* Check that we are able to do I/O */
    outw(0x8A00, 0x8A00);
    cprintf("FS can do I/O\n");

    serve_init();
    fs_init();
    fs_test();
    serve();
}
