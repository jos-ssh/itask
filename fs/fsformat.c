/*
 * JOS file system format
 */

/* We don't actually want to define off_t! */
#include "inc/filemode.h"
#define off_t xxx_off_t
#define bool  xxx_bool
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#undef off_t
#undef bool

/* Prevent inc/types.h, included from inc/fs.h,
 * from attempting to redefine types defined in the host's inttypes.h. */
#define JOS_INC_TYPES_H
/* Typedef the types that inc/mmu.h needs. */
typedef uint32_t physaddr_t;
typedef uint32_t off_t;
typedef int bool;

#include <inc/mmu.h>
#include <inc/fs.h>

#define ROUNDUP(n, v) ((n)-1 + (v) - ((n)-1) % (v))
#define MAX_DIR_ENTS  128
#define DISKSIZE      0xC0000000
#define DEFAULT_UID   1
#define DEFAULT_GID   0

struct Dir {
    struct File *f;
    struct File *ents;
    int n;
};

uint32_t nblocks;
char *diskmap, *diskpos;
struct Super *super;
uint32_t *bitmap;

static uint64_t
roundup(uint64_t n, uint64_t v) {
    if (n == 0) {
        return v;
    }
    return n - 1 + v - (n - 1) % v;
}

void
panic(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

void
readn(int f, void *out, size_t n) {
    size_t p = 0;
    while (p < n) {
        int m = read(f, out + p, n - p);
        if (m < 0)
            panic("read: %s", strerror(errno));
        if (m == 0)
            panic("read: Unexpected EOF");
        p += m;
    }
}

uint32_t
blockof(void *pos) {
    return ((char *)pos - diskmap) / BLKSIZE;
}

void *
alloc(uint32_t bytes) {
    void *start = diskpos;
    diskpos += roundup(bytes, BLKSIZE);
    if (blockof(diskpos) >= nblocks)
        panic("out of disk blocks");
    return start;
}

void
opendisk(const char *name) {
    int diskfd, nbitblocks;

    if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0)
        panic("open %s: %s", name, strerror(errno));

    if (ftruncate(diskfd, 0) < 0 || ftruncate(diskfd, nblocks * BLKSIZE) < 0)
        panic("truncate %s: %s", name, strerror(errno));

    if ((diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, diskfd, 0)) == MAP_FAILED)
        panic("mmap %s: %s", name, strerror(errno));

    close(diskfd);

    diskpos = diskmap;
    alloc(BLKSIZE);
    super = alloc(BLKSIZE);
    super->s_magic = FS_MAGIC;
    super->s_nblocks = nblocks;
    super->s_root.f_mode = IFDIR | IRWXG | IRWXO | IRWXU;
    strcpy(super->s_root.f_name, "/");

    nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
    bitmap = alloc(nbitblocks * BLKSIZE);
    memset(bitmap, 0xFF, nbitblocks * BLKSIZE);
}

void
finishdisk(void) {
    int i;

    for (i = 0; i < blockof(diskpos); ++i)
        bitmap[i / 32] &= ~(1U << (i % 32));

    if (msync(diskmap, nblocks * BLKSIZE, MS_SYNC) < 0)
        panic("msync: %s", strerror(errno));
}

void
finishfile(struct File *f, uint32_t start, uint32_t len) {
    int i;
    f->f_size = len;
    len = roundup(len, BLKSIZE);
    for (i = 0; i < len / BLKSIZE && i < NDIRECT; ++i)
        f->f_direct[i] = start + i;
    if (i == NDIRECT) {
        uint32_t *ind = alloc(BLKSIZE);
        f->f_indirect = blockof(ind);
        for (; i < len / BLKSIZE; ++i)
            ind[i - NDIRECT] = start + i;
    }
}

// Create struct Dir from File
void
startdir(struct File *f, struct Dir *dout) {
    dout->f = f;
    dout->ents = calloc(MAX_DIR_ENTS, sizeof *dout->ents);
    dout->n = 0;
}

// Add file to directory and return it
struct File *
diradd(struct Dir *d, uint32_t mode, const char *name, uint32_t uid, uint32_t gid) {
    struct File *out = &d->ents[d->n++];
    if (d->n > MAX_DIR_ENTS)
        panic("too many directory entries");
    strcpy(out->f_name, name);
    out->f_mode = mode;
    out->f_gid = gid;
    out->f_uid = uid;
    return out;
}

void
finishdir(struct Dir *d) {
    int size = d->n * sizeof(struct File);
    struct File *start = alloc(size);
    memmove(start, d->ents, size);
    finishfile(d->f, blockof(start), roundup(size, BLKSIZE));
    free(d->ents);
    d->ents = NULL;
}

void
writefile(struct Dir *dir, const char *name, uint32_t mode, uint32_t uid, uint32_t gid) {
    int fd;
    struct File *f;
    struct stat st;
    const char *last;
    char *start;

    if ((fd = open(name, O_RDONLY)) < 0)
        panic("open %s: %s", name, strerror(errno));
    if (fstat(fd, &st) < 0)
        panic("stat %s: %s", name, strerror(errno));
    if (!S_ISREG(st.st_mode))
        panic("%s is not a regular file", name);
    if (st.st_size >= MAXFILESIZE)
        panic("%s too large", name);

    last = strrchr(name, '/');
    if (last)
        last++;
    else
        last = name;

    f = diradd(dir, mode, last, uid, gid);

    start = alloc(st.st_size);
    readn(fd, start, st.st_size);
    finishfile(f, blockof(start), st.st_size);
    close(fd);
}

void
usage(void) {
    fprintf(stderr, "Usage: fsformat [fs image] NBLOCKS [objdir] [config]\n");
    exit(2);
}

int
main(int argc, char **argv) {
    char *s;
    struct Dir root;

    assert(BLKSIZE % sizeof(struct File) == 0);

    if (argc < 4)
        usage();

    nblocks = strtol(argv[2], &s, 0);
    if (*s || s == argv[2] || nblocks < 2 || nblocks > DISKSIZE / BLKSIZE)
        usage();

    opendisk(argv[1]);

    startdir(&super->s_root, &root);

    // parse objdir and config
    const char *objdir = argv[3];
    const char *config = argv[4];
    printf("config: %s, objdir %s\n", config, objdir);

    const int kbufferLength = 255;
    char buffer[kbufferLength];
    char path[kbufferLength];
    struct Dir cwd = {NULL, NULL, 0};

    FILE *cfg_file = fopen(config, "r");

    while (fgets(buffer, kbufferLength, cfg_file)) {
        if (strncmp(buffer, "#plain files", 12) == 0) {
            objdir = "";

            finishdir(&cwd);

            cwd.f = root.f;
            cwd.n = root.n;
            cwd.ents = root.ents;
        } else if (buffer[0] == '#') {
            // finish old jos directory
            if (cwd.f != NULL) {
                if (cwd.f == root.f) {
                    root = cwd;
                } else {
                    finishdir(&cwd);
                }
            }

            const char *folder_end = strchr(buffer, ' ');
            uint32_t mode = strtol(folder_end, NULL, 8);
            memset(path, 0, kbufferLength);
            strncpy(path, buffer + 1, folder_end - buffer - 1);

            struct File *fdir = diradd(&root, IFDIR | mode, path, DEFAULT_UID, DEFAULT_GID);
            startdir(fdir, &cwd);

        } else if (buffer[0] != '\n') {
            const char *path_end = strchr(buffer, ' ');
            char *num_end = NULL;

            memset(path, 0, sizeof(path));
            strcpy(path, objdir);
            strncat(path, buffer, path_end - buffer);

            uint32_t mode = strtol(path_end, &num_end, 8);
            path_end = num_end;

            uint32_t uid = strtol(path_end, &num_end, 10);
            uint32_t gid = DEFAULT_GID;
            if (uid == 0 && path_end == num_end) {
                uid = DEFAULT_UID;
            } else {
                path_end = num_end;
                gid = strtol(path_end, &num_end, 10);
                if (gid == 0 && path_end == num_end) {
                    gid = DEFAULT_GID;
                }
            }

            writefile(&cwd, path, mode, uid, gid);
        }
    }
    finishdir(&cwd);
    if (cwd.f != root.f && cwd.ents != root.ents && cwd.n != root.n) {
        finishdir(&root);
    }
    fclose(cfg_file);

    finishdisk();

    return 0;
}
