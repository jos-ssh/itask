#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

void
umain(int argc, char** argv) {
    printf("umain: envid = %d\n", thisenv->env_id);
    int res = chdir("/test");
    printf("res = %i %d\n", res, res);

    char buffer[MAXPATHLEN] = {};
    res = get_cwd(buffer, MAXPATHLEN);
    printf("res = %i %d\n", res, res);
    printf("cwd = <%s>\n", buffer);
}
