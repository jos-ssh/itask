#include <inc/assert.h>
#include <inc/lib.h>
#include <stdatomic.h>

#include "inc/env.h"
#include "inc/kmod/net.h"
#include "connection.h"
#include "net.h"

static void
recieve_from_net() {
    uint8_t status = *net->isr_status;
    if (status & VIRTIO_PCI_ISR_NOTIFY) {
        if (atomic_load(&g_Connection.state) == kReady) {
            // process connection
            cprintf("[netd-loop] try connect\n");
            while (process_receive_queue(&net->recvq) != CONNECTION_ESTABLISHED) {
                sys_yield();
            }
            cprintf("[netd-loop] connected\n");

            atomic_store(&g_Connection.state, kConnected);

        } else if (atomic_load(&g_Connection.state) == kConnected) {
            int res;
            while ((res = process_receive_queue(&net->recvq)) != NO_PACKETS) {
                if (res == CONNECTION_CLOSED) {
                    cprintf("[netd-loop] connection closed\n");
                    purge_buf(&g_Connection.recieve_buf);
                    purge_buf(&g_SendBuffer);
                    atomic_store(&g_Connection.state, kFinished);
                    break;
                }
                sys_yield();
            }
        } else if (atomic_load(&g_Connection.state) == kFinished) {
            while ((process_receive_queue(&net->recvq)) != NO_PACKETS) {
                sys_yield();
            }
            cprintf("[netd-loop] purge packets\n");
            purge_buf(&g_Connection.recieve_buf);
            purge_buf(&g_SendBuffer);
        }
    }
}

static void
send_to_net() {
    if (atomic_load(&g_Connection.state) != kConnected) {
        return;
    }

    unsigned char data[BUFSIZE];
    int res = read_buf(&g_SendBuffer, data, sizeof(size_t));
    if (res < sizeof(size_t)) {
        return;
    }
    size_t n = *(size_t *)data;
    read_buf(&g_SendBuffer, data, n);
    send_to(&g_Connection.client, (char *)data, n);
}


void
netd_process_loop(envid_t parent) {
    cprintf("[%08x: netd-loop] Starting up main loop...\n", thisenv->env_id);

    assert(parent == kmod_find_any_version(NETD_MODNAME));


    // initialize queue
    initialize();

    for (;;) {
        // process recieved from net packages
        recieve_from_net();

        // process recieved from users packages
        send_to_net();
        sys_yield();
    }
}