__attribute__((weak)) void _start() {
    asm("mov $60, %rax");  // Системный вызов exit
    asm("mov $0, %rdi");   // Передача аргумента 0 для exit
    asm("syscall");        // Вызов системного вызова
}

__attribute__((weak)) void* __ctors_start;
__attribute__((weak)) void* __ctors_end;

void syslog(int priority, const char *format, ...) {
}

void* fflush;
void* fcntl;
void* execve;
void* openlog;

void* ioctl;
void* ptsname;
void* isatty;
void* ttyname;
void* chown;


void* setsid;
void* poll;
void* getpwnam;
void* grantpt;
void* unlockpt;
void* dup2;

void* initgroups;
void* inet_pton;
void* gettimeofday;
// void* setutent;
// void* pututline;
// void* endutent;
// void* environ;
// void* execvp;
// void* setenv;
// void* unsetenv;
// void* bind;
// void* connect;
// void* getegid;
// void* fsync;
// void* unlink;
// void* rmdir;
// void* symlink;
// void* getcwd;
// void* mkfifo;


// void* usleep;

int __jos_errno_loc;