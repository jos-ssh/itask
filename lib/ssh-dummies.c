__attribute__((weak)) void _start() {
    asm("mov $60, %rax");  // Системный вызов exit
    asm("mov $0, %rdi");   // Передача аргумента 0 для exit
    asm("syscall");        // Вызов системного вызова
}

__attribute__((weak)) void* __ctors_start;
__attribute__((weak)) void* __ctors_end;

void syslog(int priority, const char *format, ...) {
}

void* stderr;
void* fflush;
void* _exit;
void* fcntl;
void* waitpid;
void* execve;
void* openlog;
void* ioctl;
void* ptsname;
void* isatty;
void* ttyname;
void* chown;
void* chdir;
void* setsid;
void* poll;
void* getpwnam;
void* grantpt;
void* unlockpt;
void* dup2;
void* getsockname;
void* getpeername;
void* geteuid;
void* setgid;
void* getgid;
void* initgroups;
void* setuid;
void* getuid;
void* inet_pton;
void* gettimeofday;
void* getpid;
void* setutent;
void* pututline;
void* sleep;
void* endutent;
void* environ;
void* getpwuid;
void* execvp;
void* setenv;
void* unsetenv;
void* socket;
void* bind;
void* connect;
void* getegid;
void* fsync;
void* unlink;
void* umask;
void* rmdir;
void* symlink;
void* getcwd;
void* mkfifo;
void* fchdir;
void* usleep;

int __jos_errno_loc;