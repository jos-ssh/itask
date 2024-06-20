#include <inc/types.h>
#include <inc/time.h>
#include <inc/stdio.h>
#include <inc/lib.h>

void
umain(int argc, char **argv) {
    char time[20];
    int now = sys_gettime();
    struct tm tnow;

    mktime(now, &tnow);

    snprint_datetime(time, 20, &tnow);
    int res = printf("DATE: %s\n", time);
    if (res < 0) {
      panic("printf: %i", res);
    }
}
