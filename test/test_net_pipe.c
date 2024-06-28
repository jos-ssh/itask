#include "inc/error.h"
#include "inc/socket.h"
#include <inc/lib.h>
#include <inc/fcntl.h>
#include <inc/poll.h>

void
run_child(int pipefd) {
    for (size_t i = 0; i < 10; ++i) {
        printf("%08x -> %zu\n", thisenv->env_id, i);
        fprintf(pipefd, "%zu", i);
        sys_yield();
        sys_yield();
        sys_yield();
    }
}

void
umain(int argc, char** argv) {
    int pipes[2];
    int child_rd, child_wr;
    int res;

    int net = opensock();
    fcntl(net, F_SETFL, O_NONBLOCK);

    res = pipe(pipes);
    if (res < 0) { panic("pipe: %i", res); }
    child_rd = pipes[0];
    child_wr = pipes[1];

    fcntl(child_rd, F_SETFL, O_NONBLOCK);

    /*
    res = fork();
    if (res < 0) { panic("fork: %i", res); }
    if (res == 0) {
        close(child_rd);
        run_child(child_wr);
        exit();
    }
    */
    close(child_wr);

    char buffer[1024];
    bool done1 = false, done2 = false;
    while (!done1 || !done2) {
        struct pollfd poll_info[] = {{child_rd, 0}, {net, POLLIN}};
        res = poll(poll_info, 2, 10000);

        if (res < 0) { panic("poll: %i", res); }
        if (res == 0) printf("poll timeout!");

        if (poll_info[0].revents & POLLIN) {
            assert(fpoll(child_rd) & POLLIN);

            res = read(child_rd, buffer, 1023);
            if (res < 0) { panic("read: %i", res); }
            if (res == 0) {
                done1 = true;
            } else {
                buffer[res] = '\0';
                printf("child: %s\n", buffer);
            }
        }
        if (poll_info[1].revents) {
            res = read(net, buffer, 1023);
            if (res < 0) { panic("recv: %i", res); }
            if (res == 0) {
                done2 = true;
            } else {
                buffer[res] = '\0';
                printf("net: %s\n", buffer);
                // break;
            }
        }
    }
}
