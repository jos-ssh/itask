/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_FS_H
#define JOS_INC_FS_H

#include <inc/types.h>
#include <inc/filemode.h>
#include <inc/mmu.h>

typedef uint32_t blockno_t;

/* File nodes (both in-memory and on-disk) */

/* Bytes per file system block - same as page size */
#define BLKSIZE    PAGE_SIZE
#define BLKBITSIZE (BLKSIZE * 8)

/* Maximum size of a filename (a single path component), including null
 * Must be a multiple of 4 */
#define MAXNAMELEN 128

/* Maximum size of a complete pathname, including null */
#define MAXPATHLEN 1024

/* Number of block pointers in a File descriptor */
#define NDIRECT 10
/* Number of direct block pointers in an indirect block */
#define NINDIRECT (BLKSIZE / 4)

#define MAXFILESIZE ((NDIRECT + NINDIRECT) * BLKSIZE)

#define SETBIT(v, n) ((v)[(n / 32)] |= 1U << ((n) % 32))
#define CLRBIT(v, n) ((v)[(n / 32)] &= ~(1U << ((n) % 32)))
#define TSTBIT(v, n) ((v)[(n / 32)] & (1U << ((n) % 32)))

/* Max count in getdents request 
 * (calculated from the fact that 
 * sizeof(struct Fsreq_getdents) < PAGE_SIZE) */
#define MAX_GETDENTS_COUNT 22

struct File {
    char f_name[MAXNAMELEN]; /* filename */
    off_t f_size;            /* file size in bytes */
    uint32_t f_uid;          /* file user id */
    uint32_t f_gid;          /* file group id */
    uint32_t f_mode;         /* file type and access rights */

    /* Block pointers. */
    /* A block is allocated iff its value is != 0. */
    blockno_t f_direct[NDIRECT]; /* direct blocks */
    blockno_t f_indirect;        /* indirect block */

    /* Pad out to 256 bytes; must do arithmetic in case we're compiling
     * fsformat on a 64-bit machine. */
    uint8_t f_pad[256 - sizeof(char) * MAXNAMELEN - sizeof(off_t) - sizeof(uint32_t) * 3 - sizeof(blockno_t) * NDIRECT - sizeof(blockno_t)];
} __attribute__((packed)); /* required only on some 64-bit machines */

/* An inode block contains exactly BLKFILES 'struct File's */
#define BLKFILES (BLKSIZE / sizeof(struct File))

/* File system super-block (both in-memory and on-disk) */

#define FS_MAGIC 0x4A0530AE /* related vaguely to 'J\0S!' */

struct Super {
    uint32_t s_magic;    /* Magic number: FS_MAGIC */
    blockno_t s_nblocks; /* Total number of blocks on disk */
    struct File s_root;  /* Root directory node */
};

/* Definitions for requests from clients to file system */
enum {
    FSREQ_OPEN = 1,
    FSREQ_SET_SIZE,
    /* Read returns a Fsret_read on the request page */
    FSREQ_READ,
    FSREQ_WRITE,
    /* Stat returns a Fsret_stat on the request page */
    FSREQ_STAT,
    FSREQ_FLUSH,
    FSREQ_REMOVE,
    FSREQ_SYNC,
    FSREQ_MKDIR,
    FSREQ_GETDENTS,
    FSREQ_CHMOD,
    FSREQ_CHOWN,
};

struct FileInfo {
    char f_name[MAXNAMELEN]; /* filename */
    off_t f_size;            /* file size in bytes */
    uint32_t f_type;         /* file type */
};

union Fsipc {
    struct Fsreq_open {
        char req_path[MAXPATHLEN];
        int req_oflags;
        uint32_t req_omode;
        uint32_t req_gid;
        uint32_t req_uid;
    } open;
    struct Fsreq_set_size {
        int req_fileid;
        off_t req_size;
    } set_size;
    struct Fsreq_read {
        int req_fileid;
        size_t req_n;
    } read;
    struct Fsret_read {
        char ret_buf[PAGE_SIZE];
    } readRet;
    struct Fsreq_write {
        int req_fileid;
        size_t req_n;
        char req_buf[PAGE_SIZE - (2 * sizeof(size_t))];
    } write;
    struct Fsreq_stat {
        int req_fileid;
    } stat;
    struct Fsret_stat {
        char ret_name[MAXNAMELEN];
        off_t ret_size;
        uint32_t ret_mode;
        uint32_t ret_uid;
        uint32_t ret_gid;
        int ret_isdir;
    } statRet;
    struct Fsreq_flush {
        int req_fileid;
    } flush;
    struct Fsreq_remove {
        char req_path[MAXPATHLEN];
    } remove;
    struct Fsreq_mkdir {
        char req_path[MAXPATHLEN]; 
        uint32_t req_gid;
        uint32_t req_uid;
        uint32_t req_omode;
    } mkdir;
    struct Fsreq_getdents {
        char req_path[MAXPATHLEN];
        int count;
        int from_which_count;
        struct FileInfo buffer[MAX_GETDENTS_COUNT];
    } getdents;
    struct Fsreq_chmod {
        char req_path[MAXPATHLEN];
        int req_mode;
    } chmod;
    struct Fsreq_chown {
        char req_path[MAXPATHLEN];
        uint64_t req_uid;
        uint64_t req_gid;
    } chown;

    /* Ensure Fsipc is one page */
    char _pad[PAGE_SIZE];
};

#endif /* !JOS_INC_FS_H */
