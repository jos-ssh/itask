#include <inc/crypto.h>
#include <inc/convert.h>
#include <inc/passw.h>
#include <inc/lib.h>

#include <inc/kmod/users.h>
#include <inc/rpc.h>

int flag[256];
uid_t current_uid = 1;
guid_t current_guid = 1;

static void
usage(void) {
    printf("usage: useradd [options] LOGIN\n\n"
           "options:\n"
           "  -d          home directory of the new account\n"
           "\n");
    exit();
}


static int
write_passw_line(struct UsersdUseradd* req, uid_t uid, guid_t guid) {
    char str_buf[100];

    int fd = open(PASSWD_PATH, O_WRONLY);
    if (fd < 0) {
        return fd;
    }
    struct Stat st;
    fstat(fd, &st);

    seek(fd, st.st_size);
    write(fd, "\n", 1);
    write(fd, req->username, strlen(req->username));

    write(fd, ":", 1);

    int length = long_to_str(uid, 10, str_buf, sizeof(str_buf));
    write(fd, str_buf, length);

    write(fd, ":", 1);

    length = long_to_str(guid, 10, str_buf, sizeof(str_buf));
    write(fd, str_buf, length);

    write(fd, ":", 1);
    write(fd, req->homedir, strlen(req->homedir));

    write(fd, ":/bin/sh", 8);

    return close(fd);
}


static int
write_shadow_line(struct UsersdUseradd* req) {

    int fd = open(SHADOW_PATH, O_WRONLY);
    if (fd < 0) {
        return fd;
    }

    struct Stat st;
    fstat(fd, &st);

    seek(fd, st.st_size);
    write(fd, "\n", 1);
    write(fd, req->username, strlen(req->username));

    write(fd, ":default:", 9);

    unsigned char hashed_passwd[KEY_LENGTH + 1];
    sys_crypto_get(req->passwd, "default", hashed_passwd);
    write(fd, hashed_passwd, KEY_LENGTH);

    write(fd, ":1024", 5);

    return close(fd);
}


static void
add_user(const char* username) {
    if (strchr(username, ':')) {
        printf("useradd: unsupported character ':'\n");
        return;
    }

    struct PasswParsed passw;
    char passwd_line_buf[kMaxLineBufLength];
    int res = find_passw_line_user(username, passwd_line_buf, kMaxLineBufLength, &passw);
    if (res == 0) {
        printf("useradd: already logged\n");
        exit();
    } else if (res != -E_NO_ENT) {
        printf("useradd: %i\n", res);
        exit();
    }

    struct UsersdUseradd request;

    strcpy(request.username, username);
    strcpy(request.homedir, "/");

    // fill /etc/passwd
    res = write_passw_line(&request, ++current_uid, current_guid);
    if (res) {
        printf("useradd: /etc/passwd: %i\n", res);
        exit();
    }

    // fill /etc/shadow
    char ask[MAXPATHLEN];
    strcpy(ask, "useradd: new password for ");
    strcat(ask, request.username);
    strcat(ask, ": ");

    do {
        strcpy(request.passwd, readline_noecho(ask));
        printf("\n");
        if (strchr(request.passwd, ':')) {
            printf("useradd: unsupported character ':'\n");
            return;
        }
    } while (strlen(request.passwd) == 0);

    res = write_shadow_line(&request);
    if (res) {
        printf("useradd: /etc/shadow: %i\n", res);
        exit();
    }
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
        add_user(argv[1]);
    } else {
        usage();
    }
}
