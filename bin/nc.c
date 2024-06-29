#include <inc/lib.h>

#include "inc/kmod/net.h"
#include "inc/socket.h"


#define ENTERC 13
#define CtrlD 4
#define CtrlC 3

void
netcat() {
    char recv_buffer[BUFSIZE];
    size_t send_len = 0;
    char send_buffer[BUFSIZE];

    int fd = opensock();
    USED(fd);

    for (;;) {
        int res = devsocket_recv(recv_buffer, -1);

        if (res == -E_CONNECTION_СLOSED) {
            printf("Connection closed\n");
            exit();
        } else if (res < 0) {
            panic("netcat failed: %i\n", res);
        } else if (res != 0) {
            printf("netcat: %s", recv_buffer);
        }
        res = getchar_unlocked();
        if (res > 0) {
            // if (res == CtrlC || res == CtrlD) {
            //     printf("Exiting...\n");
            //     exit();
            // }
            if (res == ENTERC) {
                printf("\n");
                send_buffer[send_len++] = '\n';
                devsocket_send(send_buffer, send_len);
            } else {
                printf("%c", res);
                send_buffer[send_len++] = res;
            }
        }
        sys_yield();
    }
}

void
umain(int argc, char** argv) {
    netcat();
}
