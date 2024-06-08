#include <inc/lib.h> // for open and close

#include <inc/stdio.h>
#include <inc/fd.h>


struct Fd* fopen(const char *path, int mode) {
    int res;

    int fd = open(path, mode);
    if (fd < 0)
        return NULL;

    struct Fd* stream;
    if ((res = fd_lookup(fd, &stream)) < 0)
        return NULL;

    return stream;
}

int fclose(struct Fd* stream) {
    return close(fd2num(stream));
}

int fgetc(struct Fd* stream) {
    char buf;

    if (read(fd2num(stream), &buf, 1) <= 0)
        return EOF;
    
    return buf;
}

char *fgets(char *str, int size, struct Fd *stream) {
    int c;
    int i = 0;

    while (i < size - 1) {
        c = fgetc(stream);

        if (c == EOF) {
            break;
        }

        str[i] = c;

        if (c == '\n') {
            i++;
            break;
        }

        i++;
    }

    str[i] = '\0';

    if (i == 0 && c == EOF) {
        return NULL;
    }

    return str;
}

