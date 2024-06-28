#include "inc/socket.h"
#include <inc/lib.h>

void
umain(int argc, char** argv) {
    if (argc > 1) {
        spawnl("/bin/echo", "echo", argv[1], NULL);
    }

    printf("I am %08x\n", thisenv->env_id);
    while (1) {
        int res = devsocket_poll();
        if (res >= 0) {
            printf("socket has %d bytes of data\n", res);
        } else {
            printf("poll err: %i\n", res);
        }
    }
}
