#pragma once

/* Fake signal functions.  */

/* Type of a signal handler.  */
typedef void (*__sighandler_t) (int);

#define	SIG_ERR	 ((__sighandler_t) -1)	/* Error return.  */
#define	SIG_DFL	 ((__sighandler_t)  0)	/* Default action.  */
#define	SIG_IGN	 ((__sighandler_t)  1)	/* Ignore signal.  */

/* Historical signals specified by POSIX. */
#define	SIGHUP		1	/* Hangup.  */
#define	SIGQUIT		3	/* Quit.  */
#define	SIGTRAP		5	/* Trace/breakpoint trap.  */
#define	SIGKILL		9	/* Killed.  */
#define	SIGPIPE		13	/* Broken pipe.  */
#define	SIGALRM		14	/* Alarm clock.  */

#define SIGCHLD		17	/* Child terminated or stopped.  */
