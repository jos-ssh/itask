#include <inc/unistd.h>
#include <inc/signal.h>

#include <inc/rpc.h>
#include <inc/kmod/signal.h>
#include <inc/lib.h>


static void
notify_parent() {
    kill(thisenv->env_parent_id, SIGCHLD);
}

[[noreturn]] void
exit(void) {
    close_all();
    notify_parent();
    sys_env_destroy(0);
    exit(); // so surpress warnig =)
}

[[noreturn]] void
_exit(int status) {
    printf("Exit\n");
    (void)status;
    exit();
}


int setuid(uid_t uid) NOTIMPLEMENTED(int);
int setgid(gid_t gid) NOTIMPLEMENTED(int);

uid_t geteuid(void) NOTIMPLEMENTED(uid_t);
uid_t getuid(void) NOTIMPLEMENTED(uid_t);
gid_t getgid(void) NOTIMPLEMENTED(uid_t);
pid_t getpid(void) NOTIMPLEMENTED(pid_t);


int chdir(const char *path) NOTIMPLEMENTED(int);
int fchdir(int fd) NOTIMPLEMENTED(int);
int fsync(int fd) NOTIMPLEMENTED(int);
char *getcwd(char buf, size_t size) NOTIMPLEMENTED(char *);
int dup2(int oldfd, int newfd) NOTIMPLEMENTED(int);
int chown(const char *pathname, uid_t owner, gid_t group) NOTIMPLEMENTED(int);

pid_t setsid(void) NOTIMPLEMENTED(pid_t);

char *ttyname(int fd) NOTIMPLEMENTED(char *);

int grantpt(int fd) NOTIMPLEMENTED(int);
int unlockpt(int fd) NOTIMPLEMENTED(int);
char *ptsname(int fd) NOTIMPLEMENTED(int);


unsigned int sleep(unsigned int seconds) NOTIMPLEMENTED(unsigned int);

int isatty(int fd) NOTIMPLEMENTED(int);
int execve(const char *pathname, char *const argv[],
           char *const envp[]) NOTIMPLEMENTED(int);
