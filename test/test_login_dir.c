#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>

void test_set_get_cwd() {
    printf("=========test_set_get_cwd=========\n");
    printf("test_set_get_cwd: envid = %d\n", thisenv->env_id);
    int res = chdir("/test");
    printf("test_set_get_cwd: res = %i %d\n", res, res);

    char buffer[MAXPATHLEN] = {};
    res = get_cwd(buffer, MAXPATHLEN);
    printf("test_set_get_cwd: res = %i %d\n", res, res);
    printf("test_set_get_cwd: cwd = <%s>\n", buffer);
    
}

void test_chown() {
    printf("=========test_chown=========\n");

    int res = chown("lol_error", 0, 0);
    printf("test_chown: res = %i %d\n", res, res);

    res = chown("test_login_dir", 100, 100);
    printf("test_chown: res = %i %d\n", res, res);
}

void
umain(int argc, char** argv) {
    test_set_get_cwd();
    test_chown();
}
