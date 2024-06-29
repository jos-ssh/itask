#pragma once

#include <inc/types.h>

/* File open modes */
#define O_RDONLY  0x0000 /* open for reading only */
#define O_WRONLY  0x0001 /* open for writing only */
#define O_RDWR    0x0002 /* open for reading and writing */
#define O_ACCMODE 0x0003 /* mask for above modes */

#define O_CREAT 0x0100 /* create if nonexistent */
#define O_TRUNC 0x0200 /* truncate to zero length */
#define O_EXCL  0x0400 /* error if already exists */
#define O_MKDIR 0x0800 /* create directory, not regular file */

#define O_NOCTTY 0xBAD

int setuid(uid_t uid);
int setgid(gid_t gid);

uid_t geteuid(void);
uid_t getuid(void);
gid_t getgid(void);

pid_t getpid(void);

int chdir(const char *path);
int fchdir(int fd);
int fsync(int fd);
char *getcwd(char *buf, size_t size);
int dup2(int oldfd, int newfd);
int chown(const char *pathname, uid_t owner, gid_t group);

pid_t setsid(void);

char *ttyname(int fd);

// man says it's from stdlid.h
int grantpt(int fd);
int unlockpt(int fd);
char *ptsname(int fd);

unsigned int sleep(unsigned int seconds);

int isatty(int fd);
int execve(const char *pathname, char *const argv[],
           char *const envp[]);

__attribute__((noreturn)) void
_exit(int status);