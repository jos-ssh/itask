/* Public definitions for the POSIX-like file type and mode types */

#ifndef JOS_INC_STAT_H
#define JOS_INC_STAT_H

#define IUMASK 0002 // default umask value (rw-rw-r--)

#define IFMT 0170000 // bit mask for the file type bit field

#define IFREG  0100000 // regular file
#define IFDIR  0040000 // directory
#define IFIFO  0010000 // pipe
#define IFSOCK 0140000 // socket

#define ISREG(m)  (((m) & IFMT) == IFREG)
#define ISDIR(m)  (((m) & IFMT) == IFDIR)
#define ISFIFO(m) (((m) & IFMT) == IFIFO)
#define ISSOCK(m) (((m) & IFMT) == IFSOCK)


#define ISUID 04000 // set-user-ID bit
#define ISGID 02000 // set-group-ID bit
#define ISVTX 01000 // sticky bit

#define IRWXU 00700 // owner has read, write, and execute permission

#define IRUSR 00400 // owner has read permission
#define IWUSR 00200 // owner has write permission
#define IXUSR 00100 // owner has execute permission

#define IRWXG 00070 // group has read, write, and execute permission

#define IRGRP 00040 // group has read permission
#define IWGRP 00020 // group has write permission
#define IXGRP 00010 // group has execute permission

#define IRWXO 00007 // others (not in group) have read, write, and execute permission

#define IROTH 00004 // others have read permission
#define IWOTH 00002 // others have write permission
#define IXOTH 00001 // others have execute permission

#endif
