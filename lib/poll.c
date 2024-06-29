#include <inc/poll.h>

#include <inc/lib.h>

int
poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    size_t start_time = vsys_gettime();
    size_t elapsed = 0;

    while (timeout < 0 || elapsed <= timeout / 1000) {
        size_t event_count = 0;

        // printf("poll: probing fds\n");
        for (size_t i = 0; i < nfds; ++i) {
            fds[i].revents = 0;

            // printf("poll: probing fd %zu\n", i);
            int flags = fpoll(fds[i].fd);
            // printf("poll: probing fd %zu done\n", i);
            if (flags < 0) {
                fds[i].revents |= POLLNVAL;
                return flags;
            }
            int event_filter = fds[i].events | POLLHUP | POLLERR;

            fds[i].revents |= flags & event_filter;
            if (fds[i].revents) {
                ++event_count;
            }
        }
        // printf("poll: probing fds done\n");

        if (event_count > 0 || timeout == 0) {
            return event_count;
        }

        elapsed = vsys_gettime() - start_time;
    }

    return 0;
}
