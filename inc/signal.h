#pragma once

#include <inc/types.h>

/* signal.c */
unsigned int alarm(unsigned int seconds);
int kill(envid_t env, int sig_no);

typedef void (*sighandler_t)(int);
sighandler_t signal(int signo, sighandler_t handler);

/* Fake signal functions.  */

#define SIG_ERR ((sighandler_t)-1) /* Error return */
#define SIG_DFL ((sighandler_t)0)  /* Default action */
#define SIG_IGN ((sighandler_t)1)  /* Ignore signal */

enum Signal {
    SIGALRM,
    SIGKILL,
    SIGTERM,
    SIGCHLD,
    SIGPIPE,

    NSIGNAL,
};
