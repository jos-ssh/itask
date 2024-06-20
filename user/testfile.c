#include "inc/fs.h"
#include <inc/lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd *)0xA000000)


static int
xopen2(const char *path, int flags, uint32_t omode) {
    extern union Fsipc fsipcbuf;
    envid_t fsenv;

    strcpy(fsipcbuf.open.req_path, path);
    fsipcbuf.open.req_oflags = flags;
    fsipcbuf.open.req_omode = omode;

    fsenv = ipc_find_env(ENV_TYPE_FS);
    size_t sz = PAGE_SIZE;
    ipc_send(fsenv, FSREQ_OPEN, &fsipcbuf, sz, PROT_RW);
    return ipc_recv(NULL, FVA, &sz, NULL);
}

static int
xopen(const char *path, int flags) {
    return xopen2(path, flags, 0);
}

void
increment_file_name(char *buffer, size_t buf_size) {
    for (size_t i = 0; i < buf_size; ++i) {
        if (buffer[i] < 'z') {
            if (buffer[i] == 0) {
                buffer[i] = 'a';
            } else {
                ++buffer[i];
            }
            return;
        }
    }
}

void
umain(int argc, char **argv) {
    int64_t r, f;
    struct Fd *fd;
    struct Fd fdcopy;
    struct Stat st;
    char buf[512];

    /* We open files manually first, to avoid the FD layer */
    if ((r = xopen("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
        panic("serve_open /not-found: %ld", (long)r);
    else if (r >= 0)
        panic("serve_open /not-found succeeded!");

    if ((r = xopen("/newmotd", O_RDONLY)) < 0)
        panic("serve_open /newmotd: %ld", (long)r);
    if (FVA->fd_dev_id != 'f' || FVA->fd_offset != 0 || FVA->fd_omode != O_RDONLY)
        panic("serve_open did not fill struct Fd correctly\n");
    cprintf("serve_open is good\n");

    if ((r = devfile.dev_stat(FVA, &st)) < 0)
        panic("file_stat: %ld", (long)r);
    if (strlen(msg) != st.st_size)
        panic("file_stat returned size %ld wanted %zd\n", (long)st.st_size, strlen(msg));
    cprintf("file_stat is good\n");

    memset(buf, 0, sizeof buf);
    if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
        panic("file_read: %ld", (long)r);
    if (strcmp(buf, msg) != 0)
        panic("file_read returned wrong data");
    cprintf("file_read is good\n");

    if ((r = devfile.dev_close(FVA)) < 0)
        panic("file_close: %ld", (long)r);
    cprintf("file_close is good\n");

    /* We're about to unmap the FD, but still need a way to get
     * the stale filenum to serve_read, so we make a local copy.
     * The file server won't think it's stale until we unmap the
     * FD page. */
    fdcopy = *FVA;
    sys_unmap_region(0, FVA, PAGE_SIZE);

    if ((r = devfile.dev_read(&fdcopy, buf, sizeof buf)) != -E_INVAL)
        panic("serve_read does not handle stale fileids correctly: %ld", (long)r);
    cprintf("stale fileid is good\n");

    /* Try writing */
    if ((r = xopen2("/new-file", O_RDWR | O_CREAT, IRWXU | IRWXG | IRWXO)) < 0)
        panic("serve_open /new-file: %ld", (long)r);

    if ((r = devfile.dev_write(FVA, msg, strlen(msg))) != strlen(msg))
        panic("file_write: %ld", (long)r);
    cprintf("file_write is good\n");

    FVA->fd_offset = 0;
    memset(buf, 0, sizeof buf);
    if ((r = devfile.dev_read(FVA, buf, sizeof buf)) < 0)
        panic("file_read after file_write: %ld", (long)r);
    if (r != strlen(msg))
        panic("file_read after file_write returned wrong length: %ld", (long)r);
    if (strcmp(buf, msg) != 0)
        panic("file_read after file_write returned wrong data");
    cprintf("file_read after file_write is good\n");

    /* Now we'll try out open */
    if ((r = open("/not-found", O_RDONLY)) < 0 && r != -E_NOT_FOUND)
        panic("open /not-found: %ld", (long)r);
    else if (r >= 0)
        panic("open /not-found succeeded!");

    if ((r = open("/newmotd", O_RDONLY)) < 0)
        panic("open /newmotd: %ld", (long)r);
    fd = (struct Fd *)(0xD0000000 + r * PAGE_SIZE);
    if (fd->fd_dev_id != 'f' || fd->fd_offset != 0 || fd->fd_omode != O_RDONLY)
        panic("open did not fill struct Fd correctly\n");
    cprintf("open is good\n");

    /* Try files with indirect blocks */
    if ((f = open("/big", O_WRONLY | O_CREAT)) < 0)
        panic("creat /big: %ld", (long)f);
    memset(buf, 0, sizeof(buf));
    for (int64_t i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
        *(int *)buf = i;
        if ((r = write(f, buf, sizeof(buf))) < 0)
            panic("write /big@%ld: %ld", (long)i, (long)r);
    }
    close(f);

    if ((f = open("/big", O_RDONLY)) < 0)
        panic("open /big: %ld", (long)f);
    for (int64_t i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
        *(int *)buf = i;
        if ((r = readn(f, buf, sizeof(buf))) < 0)
            panic("read /big@%ld: %ld", (long)i, (long)r);
        if (r != sizeof(buf))
            panic("read /big from %ld returned %ld < %d bytes",
                  (long)i, (long)r, (uint32_t)sizeof(buf));
        if (*(int *)buf != i)
            panic("read /big from %ld returned bad data %d",
                  (long)i, *(int *)buf);
    }
    close(f);

    char name_buffer[MAXNAMELEN + 1];
    memset(name_buffer, 0, sizeof(name_buffer));
    name_buffer[0] = '/';

    // for (size_t file_name = 1; file_name <= 0xA0000 * BLKSIZE / MAXFILESIZE; ++file_name)
    for (size_t file_name = 1; file_name <= 10; ++file_name) {
        increment_file_name(name_buffer + 1, MAXNAMELEN);
        cprintf("writing %s (%lu/%lld)\n", name_buffer, file_name, 0xA0000 * BLKSIZE / MAXFILESIZE);
        if ((f = open(name_buffer, O_RDWR | O_CREAT, IRWXU | IRWXG | IRWXO)) < 0) {
            panic("open %s: %ld", name_buffer, f);
        }
        memset(buf, 0, sizeof(buf));
        for (int64_t i = 0; i < MAXFILESIZE; i += sizeof(buf)) {
            *(int *)buf = i;
            if ((r = write(f, buf, sizeof(buf))) < 0)
                panic("write %s@%ld: %ld", name_buffer, (long)i, (long)r);
        }
        close(f);
    }

    cprintf("large file is good\n");
}
