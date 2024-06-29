#include <inc/lib.h>

#include <inc/crypto.h>
#include <inc/convert.h>
#include <inc/passw.h>
#include <inc/lib.h>

#include <inc/kmod/users.h>
#include <inc/rpc.h>


#define TOO_MATCH 1<<10
int flag[256];

static int
copy_without_user(char* dst, const char* src, const char* username) {
    const int buffer_size = strlen(src);
    int found_line = -1;
    int i = 0;
    int line_n = 0;
    while(i < buffer_size) {    
        if (found_line == -1) {
            const int old_i = i;
            while(i < buffer_size && src[i] != ':')
                i++;

            if (!strncmp(username, &src[old_i], i - old_i)) {
                found_line = line_n;

                while (i < buffer_size && src[i] != '\n')
                    i++;
                
                i++;
                if (i >= buffer_size)
                    break;

                strcpy(&dst[old_i], &src[i]);                
                break;
            }

            i = old_i;
        }

        while (i < buffer_size && src[i] != '\n') {
            dst[i] = src[i];
            i++;
        }

        // skip '\n'
        if (i < buffer_size)
            dst[i] = src[i];
        i++;
        line_n++;
    }

    if (found_line == -1) {
        printf("userdel: user do not exist\n");
        exit();
    }

    return found_line;
}

// return number line, on what username find
static int
del_line_from_passwrd(const char* username) {
    int fd = open(PASSWD_PATH, O_RDONLY);
    if (fd < 0) {
        printf("error during file open %i %d\n", fd, fd);
        exit();
    }
    struct Stat st;
    fstat(fd, &st);

    char buffer[TOO_MATCH] = {};
    read(fd, buffer, st.st_size);

    close(fd);

    char second_buffer[TOO_MATCH] = {};
    int line = copy_without_user(second_buffer, buffer, username);
    if (line < 0) {
        printf("userdel: user do not exist\n");
        exit();
    }

    printf("before: <%s>\n", buffer);
    printf("after: <%s>\n", second_buffer);
    
    fd = open(PASSWD_PATH, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        printf("error during file open %i %d\n", fd, fd);
        exit();
    }
    write(fd, second_buffer, strlen(second_buffer));

    return line;
}

static void 
del_line_from_shadow(int line_index) {
    // TODO implement
}

static void
del_user(const char* username) {
    int line = del_line_from_passwrd(username);
    del_line_from_shadow(line);
}


static void
usage(void) {
    printf("usage: userdel [options] LOGIN\n\n"
           "options:\n"
           "\n");
    exit();
}

void
umain(int argc, char** argv) {
    int i;
    struct Argstate args;

    argstart(&argc, argv, &args);
    while ((i = argnext(&args)) >= 0) {
        switch (i) {
        case 'd':
            flag[i]++;
            break;
        case 'h':
        default:
            usage();
        }
    }
    if (argc == 2) {
        del_user(argv[1]);
    } else {
        usage();
    }
}
