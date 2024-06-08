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
    return close(stream->fd_file.id);
}

int fgetc(struct Fd* stream) {
    char buf;

    if (readn(stream->fd_file.id, &buf, 1) < 0)
        return EOF;
    
    return buf;
}

char *fgets(char *s, int size, struct Fd* stream) {
    int i = 0, c = 0;

    while(--size > 0 && c > EOF && c != '\n') {
        c = fgetc(stream);

        if (c != EOF && c >= 0)
            s[i++] = c;
        }

    if(size == 0)
        return NULL;

    s[i] = '\0';

    return s;
}
