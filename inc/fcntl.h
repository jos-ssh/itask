#pragma once

/* Values for the second argument to `fcntl'.  */
#define F_SETFD 2 /* Set file descriptor flags.  */
#define F_GETFL 3 /* Get file status flags.  */
#define F_SETFL 4 /* Set file status flags.  */

#define O_NONBLOCK 0x1000 /* non-blocking IO */

/* Flags available to set through fcntl */
#define O_MOD_FLAGS (O_NONBLOCK)

int fcntl(int fd, int cmd, ... /* arg */);
