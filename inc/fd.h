/* Public definitions for the POSIX-like file descriptor emulation layer
 * that our user-land support library implements for the use of applications.
 * See the code in the lib directory for the implementation details. */

#ifndef JOS_INC_FD_H
#define JOS_INC_FD_H

#include <inc/types.h>
#include <inc/fs.h>

/* Maximum number of file descriptors a program may hold open concurrently */
#define MAXFD 32
/* Bottom of file descriptor area */
#define FDTABLE 0xD0000000LL
/* Bottom of file data area.  We reserve one data page for each FD,
 * which devices can use if they choose. */
#define FILEDATA (FDTABLE + MAXFD * PAGE_SIZE)

struct Fd;
struct Stat;
struct Dev;

/* Per-device-class file descriptor operations */
struct Dev {
    int dev_id;
    const char *dev_name;
    ssize_t (*dev_read)(struct Fd *fd, void *buf, size_t len);
    ssize_t (*dev_write)(struct Fd *fd, const void *buf, size_t len);
    int (*dev_close)(struct Fd *fd);
    int (*dev_stat)(struct Fd *fd, struct Stat *stat);
    int (*dev_trunc)(struct Fd *fd, off_t length);
    int (*dev_poll)(struct Fd *fd);
};

struct FdFile {
    int id;
};

struct FdSock {
    bool is_closed;
};

struct Fd {
    int fd_dev_id;
    off_t fd_offset;
    int fd_omode;
    union {
        /* File server files */
        struct FdFile fd_file;
        struct FdSock fd_sock;
    };
};

struct Stat {
    char st_name[MAXNAMELEN];
    off_t st_size;
    int st_isdir;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    struct Dev *st_dev;
};

char *fd2data(struct Fd *fd);
uint64_t fd2num(struct Fd *fd);
int fd_alloc(struct Fd **fd_store);
int fd_close(struct Fd *fd, bool must_exist);
int fd_lookup(int fdnum, struct Fd **fd_store);
int dev_lookup(int devid, struct Dev **dev_store);

extern struct Dev devfile;
extern struct Dev devcons;
extern struct Dev devpipe;
extern struct Dev devsock;

#endif /* not JOS_INC_FD_H */
