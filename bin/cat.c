#include <inc/lib.h>

char buf[8192];

void
cat(int f, char *s) {
    long n;
    int r;

    while ((n = read(f, buf, (long)sizeof(buf))) > 0)
        if ((r = write(1, buf, n)) != n) {
            printf("cat: write %s: %i", s, r);
            exit();
        }
    if (n < 0) {
        printf("cat: read %s: %i", s, (int)n);
        exit();
    }
}

void
umain(int argc, char **argv) {
    int f, i;

    binaryname = "cat";
    if (argc == 1)
        cat(0, "<stdin>");
    else
        for (i = 1; i < argc; i++) {
            f = open(argv[i], O_RDONLY);
            if (f < 0)
                printf("cat %s: %i\n", argv[i], f);
            else {
                cat(f, argv[i]);
                close(f);
            }
        }
}
