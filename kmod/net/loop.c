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
        if (atomic_load(&g_Connection.state) == kCreated) {
            // process connection
            cprintf("[netd-loop] try connect\n");
            while (process_receive_queue(&net->recvq) != CONNECTION_ESTABLISHED) {
                ;
            }
            cprintf("[netd-loop] connected\n");

            atomic_store(&g_Connection.state, kConnected);

        } else if (atomic_load(&g_Connection.state) == kConnected) {
            int res = process_receive_queue(&net->recvq);
            if (res == CONNECTION_CLOSED) {
                atomic_store(&g_Connection.state, kFinished);
            }
            if (res == RECIEVE_PROCESSED) {
                char buf[BUFSIZE];
                read_buf(&g_Connection.recieve_buf, buf, BUFSIZE);
                cprintf("[netd-loop] data recieved: %s\n", buf);
            }
        }
    }
}

static void
send_to_net() {
    if (atomic_load(&g_Connection.state) != kConnected) {
        return;
    }

    // TODO:
    // достать из очереди необходимые для отправки данные
    char *data = NULL;
    size_t n = 0;
    send_to(&g_Connection.client, data, n);
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