#include <inc/lib.h>

int flag[256];

#define clear "\x1b[0m"
#define green "\x1b[1;32m"
#define blue  "\x1b[1;34m"
const size_t padding = 20;

void lsdir(const char *, const char *);
void ls1(const char *, struct Stat, off_t, const char *);

void
ls(const char *path, const char *prefix) {
    int r;
    struct Stat st;

    if ((r = stat(path, &st)) < 0) {
        printf("ls: stat error with %s: %i\n", path, r);
        exit();
    }

    if (st.st_isdir && !flag['d'])
        lsdir(path, prefix);
    else
        ls1(0, st, st.st_size, path);
}

void
lsdir(const char *path, const char *prefix) {
    struct FileInfo file[MAX_GETDENTS_COUNT];

    int res = getdents(path, file, MAX_GETDENTS_COUNT);
    if (res) {
        panic("open %s: %i", path, res);
    }

    for (size_t i = 0; file[i].f_name[0]; ++i) {
        char full_path[MAXPATHLEN];
        struct Stat st;
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, file[i].f_name);

        int res = stat(full_path, &st);
        if (res) {
            printf("stat: %s: %i\n", full_path, res);
        } else {
            ls1(prefix, st, st.st_size, file[i].f_name);
        }
    }
}


static void
print_mode(uint32_t mode) {
    printf("%c", ISDIR(mode) ? 'd' : '-');
    printf("%c%c%c", mode & IRUSR ? 'r' : '-', mode & IWUSR ? 'w' : '-', mode & ISUID ? 's' : (mode & IXUSR ? 'x' : '-'));
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
ls1(const char *prefix, struct Stat st, off_t size, const char *name) {
    const char *sep;

    if (flag['l']) {
        print_mode(st.st_mode);

        printf("  %3d %3d", st.st_uid, st.st_gid);
        printf(" %8d ", size);
    }
    if (prefix) {
        if (prefix[0] && prefix[strlen(prefix) - 1] != '/')
            sep = "/";
        else
            sep = "";
        printf("%s%s", prefix, sep);
    }
    print_colored(name, st.st_mode);
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

    if (argc == 1) {
        char cwd[MAXPATHLEN];
        int res = get_cwd(cwd, MAXPATHLEN);
        if (res) {
            panic("get_cwd: %i", res);
        }
        ls(cwd, "");
    } else {
        for (i = 1; i < argc; i++)
            ls(argv[i], argv[i]);
    }
}
