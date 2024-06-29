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
    }
}

void
umain(int argc, char** argv) {
    int pipes[2];
    int child1_rd, child1_wr, child2_rd, child2_wr;
    int res;

    res = pipe(pipes);
    if (res < 0) { panic("pipe: %i", res); }
    child1_rd = pipes[0];
    child1_wr = pipes[1];

    res = pipe(pipes);
    if (res < 0) { panic("pipe: %i", res); }
    child2_rd = pipes[0];
    child2_wr = pipes[1];

    fcntl(child1_rd, F_SETFL, O_NONBLOCK);
    fcntl(child2_rd, F_SETFL, O_NONBLOCK);

    res = fork();
    if (res < 0) { panic("fork: %i", res); }
    if (res == 0) {
        close(child1_rd);
        run_child(child1_wr);
        exit();
    }
    close(child1_wr);

    res = fork();
    if (res < 0) { panic("fork: %i", res); }
    if (res == 0) {
        close(child2_rd);
        run_child(child2_wr);
        exit();
    }
    close(child2_wr);

    char buffer[1024];
    bool done1 = false, done2 = false;
    while (!done1 || !done2) {
        struct pollfd poll_info[] = {{child1_rd, POLLIN}, {child2_rd, POLLIN}};
        res = poll(poll_info, 2, 10000);
        if (res < 0) { panic("poll: %i", res); }
        if (res == 0) printf("poll timeout!");

        if (poll_info[0].revents & POLLIN) {
            assert(fpoll(child1_rd) & POLLIN);

            res = read(child1_rd, buffer, 1023);
            if (res < 0) { panic("read: %i", res); }
            if (res == 0) {
                done1 = true;
            } else {
                buffer[res] = '\0';
                printf("child1: %s\n", buffer);
            }
        }

        if (poll_info[1].revents & POLLIN) {
            assert(fpoll(child2_rd) & POLLIN);
            res = read(child2_rd, buffer, 1024);
            if (res < 0) { panic("read: %i", res); }
            if (res == 0) {
                done2 = true;
            } else {
                buffer[res] = '\0';
                printf("child2: %s\n", buffer);
            }
        }
    }
}
