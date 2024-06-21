#include <inc/lib.h>

int flag[256];

#define clear "\x1b[0m"
#define green "\x1b[1;32m"
#define blue  "\x1b[1;34m"

void lsdir(const char *, const char *);
void ls1(const char *, uint32_t, off_t, const char *);

void
ls(const char *path, const char *prefix) {
    int r;
    struct Stat st;

    if ((r = stat(path, &st)) < 0)
        panic("stat %s: %i", path, r);
    if (st.st_isdir && !flag['d'])
        lsdir(path, prefix);
    else
        ls1(0, st.st_mode, st.st_size, path);
}

void
lsdir(const char *path, const char *prefix) {
    int fd, n;
    struct File f;

    if ((fd = open(path, O_RDONLY)) < 0)
        panic("open %s: %i", path, fd);
    while ((n = readn(fd, &f, sizeof f)) == sizeof f) {
        if (f.f_name[0]) {
            ls1(prefix, f.f_mode, f.f_size, f.f_name);
        }
    }
    if (n > 0)
        panic("short read in directory %s", path);
    if (n < 0)
        panic("error reading directory %s: %i", path, n);
}


static void
print_mode(uint32_t mode) {
    printf("%c", ISDIR(mode) ? 'd' : '-');
    printf("%c%c%c", mode & IRUSR ? 'r' : '-', mode & IWUSR ? 'w' : '-', mode & IXUSR ? 'x' : '-');
    printf("%c%c%c", mode & IRGRP ? 'r' : '-', mode & IWGRP ? 'w' : '-', mode & IXGRP ? 'x' : '-');
    printf("%c%c%c", mode & IROTH ? 'r' : '-', mode & IWOTH ? 'w' : '-', mode & IXOTH ? 'x' : '-');
}

static void
print_colored(const char *file_name, uint32_t mode) {
    if (ISDIR(mode)) {
        printf(blue "%s", file_name);
        if (flag['F']) {
            printf("/");
        }
        printf(clear);
    } else {
        if (mode & IXUSR) {
            printf(green "%s" clear, file_name);
        } else {
            printf("%s" clear, file_name);
        }
    }
}


void
ls1(const char *prefix, uint32_t mode, off_t size, const char *name) {
    const char *sep;

    if (flag['l']) {
        print_mode(mode);
        printf(" %8d ", size);
    }
    if (prefix) {
        if (prefix[0] && prefix[strlen(prefix) - 1] != '/')
            sep = "/";
        else
            sep = "";
        printf("%s%s", prefix, sep);
    }
    print_colored(name, mode);
    printf("\n");
}

void
usage(void) {
    printf("usage: ls [-dFl] [file...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    struct Argstate args;

    argstart(&argc, argv, &args);
    while ((i = argnext(&args)) >= 0)
        switch (i) {
        case 'd':
        case 'F':
        case 'l':
            flag[i]++;
            break;
        default:
            usage();
        }

    if (argc == 1)
        ls("/", "");
    else {
        for (i = 1; i < argc; i++)
            ls(argv[i], argv[i]);
    }
}
