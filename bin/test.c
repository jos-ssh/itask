#include <inc/lib.h>


void
umain(int argc, char **argv) {

    if (argc != 2) {
        printf("error");
        exit();
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd >= 0) {
        struct Stat st;
        fstat(fd, &st);
        printf("fstat: uid %d guid %d mode %d\n", st.st_uid, st.st_gid, st.st_mode);

        stat(argv[1], &st);
        printf("stat: uid %d guid %d mode %d\n", st.st_uid, st.st_gid, st.st_mode);
        close(fd);
    }
}