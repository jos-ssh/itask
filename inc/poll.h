#pragma once

/* Type used for the number of file descriptors.  */
typedef unsigned long int nfds_t;

struct pollfd {
    int fd;
    short events;
    short revents;
};

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN  0x001 /* There is data to read.  */
#define POLLPRI 0x002 /* There is urgent data to read.  */
#define POLLOUT 0x004 /* Writing now will not block.  */


/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR  0x008 /* Error condition.  */
#define POLLHUP  0x010 /* Hung up.  */
#define POLLNVAL 0x020 /* Invalid polling request.  */

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
