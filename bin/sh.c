#include "inc/convert.h"
#include "inc/passw.h"
#include "inc/rpc.h"
#include <inc/lib.h>

#define BUFSIZ 1024 /* Find the buffer overrun bug! */
static char PATH[BUFSIZ] = {"/bin/"};
#define clear "\x1b[0m"
#define green "\x1b[1;32m"
#define blue  "\x1b[1;34m"

#define RECEIVE_ADDR 0x0FFFF000

/* gettoken(s, 0) prepares gettoken for subsequent calls and returns 0.
 * gettoken(0, token) parses a shell token from the previously set string,
 * null-terminates that token, stores the token pointer in '*token',
 * and returns a token ID (0, '<', '>', '|', or 'w').
 * Subsequent calls to 'gettoken(0, token)' will return subsequent
 * tokens from the string. */
int gettoken(char *s, char **token);

/* Parse a shell command from string 's' and execute it.
 * Do not return until the shell command is finished.
 * runcmd() is called in a forked child,
 * so it's OK to manipulate file descriptor state. */
#define MAXARGS 16
void
runcmd(char *s) {
    char *argv[MAXARGS], *t;
    int argc, c, i, r, p[2], fd, pipe_child;

    pipe_child = 0;
    gettoken(s, 0);

again:
    argc = 0;
    while (1) {
        switch ((c = gettoken(0, &t))) {
        case 'w': /* Add an argument */
            if (argc == MAXARGS) {
                cprintf("too many arguments\n");
                exit();
            }
            argv[argc++] = t;
            break;
        case '<': /* Input redirection */
            /* Grab the filename from the argument list */
            if (gettoken(0, &t) != 'w') {
                cprintf("syntax error: < not followed by word\n");
                exit();
            }
            /* Open 't' for reading as file descriptor 0
             * (which environments use as standard input).
             * We can't open a file onto a particular descriptor,
             * so open the file as 'fd',
             * then check whether 'fd' is 0.
             * If not, dup 'fd' onto file descriptor 0,
             * then close the original 'fd'. */
            if ((fd = open(t, O_RDONLY)) < 0) {
                cprintf("open %s for read: %i\n", t, fd);
                exit();
            }
            if (fd != 0) {
                dup(fd, 0);
                close(fd);
            }
            break;

        case '>': /* Output redirection */
            /* Grab the filename from the argument list */
            if (gettoken(0, &t) != 'w') {
                cprintf("syntax error: > not followed by word\n");
                exit();
            }
            if ((fd = open(t, O_WRONLY | O_CREAT | O_TRUNC, IRUSR | IWUSR | IRGRP | IROTH)) < 0) {
                cprintf("open %s for write: %i\n", t, fd);
                exit();
            }
            if (fd != 1) {
                dup(fd, 1);
                close(fd);
            }
            break;

        case '|': /* Pipe */
            if ((r = pipe(p)) < 0) {
                cprintf("pipe: %i", r);
                exit();
            }
            if (debug) cprintf("PIPE: %d %d\n", p[0], p[1]);
            if ((r = fork()) < 0) {
                cprintf("fork: %i", r);
                exit();
            }
            if (r == 0) {
                if (p[0] != 0) {
                    dup(p[0], 0);
                    close(p[0]);
                }
                close(p[1]);
                goto again;
            } else {
                pipe_child = r;
                if (p[1] != 1) {
                    dup(p[1], 1);
                    close(p[1]);
                }
                close(p[0]);
                goto runit;
            }
            printf("sh: | not implemented");
            exit();

        case 0: /* String is complete */
            /* Run the current command! */
            goto runit;

        default:
            printf("sh: bad token %c\n", c);
            exit();
        }
    }

runit:
    /* Return immediately if command line was empty. */
    if (argc == 0) {
        if (debug) cprintf("EMPTY COMMAND\n");
        return;
    }

    argv[argc] = 0;

    /* Print the command. */
    if (debug) {
        cprintf("[%08x] SPAWN:", thisenv->env_id);
        for (i = 0; argv[i]; i++)
            cprintf(" %s", argv[i]);
        cprintf("\n");
    }

    /* Spawn the command! */
    if ((r = spawn(argv[0], (const char **)argv)) < 0) {
        if (r != -E_NOT_FOUND) {
            printf("sh: spawn: %s: %i\n", argv[0], r);
            exit();
        }
        /* Try add PATH*/
        char cmd[BUFSIZ];
        strcpy(cmd, PATH);
        strcat(cmd, argv[0]);

        if ((r = spawn(cmd, (const char **)argv)) < 0) {
            printf("sh: spawn: %s: %i\n", cmd, r);
        }
    }

    /* In the parent, close all file descriptors and wait for the
     * spawned command to exit. */
    close_all();
    if (r >= 0) {
        if (debug) cprintf("[%08x] WAIT %s %08x\n", thisenv->env_id, argv[0], r);
        wait(r);
        if (debug) cprintf("[%08x] wait finished\n", thisenv->env_id);
    }

    /* If we were the left-hand part of a pipe,
     * wait for the right-hand part to finish. */
    if (pipe_child) {
        if (debug) cprintf("[%08x] WAIT pipe_child %08x\n", thisenv->env_id, pipe_child);
        wait(pipe_child);
        if (debug) cprintf("[%08x] wait finished\n", thisenv->env_id);
    }

    /* Done! */
    exit();
}

/* Get the next token from string s.
 * Set *p1 to the beginning of the token and *p2 just past the token.
 * Returns
 *    0 for end-of-string;
 *    < for <;
 *    > for >;
 *    | for |;
 *    w for a word.
 *
 * Eventually (once we parse the space where the \0 will go),
 * words get nul-terminated. */

#define WHITESPACE " \t\r\n"
#define SYMBOLS    "<|>&;()"

int
_gettoken(char *s, char **p1, char **p2) {
    int t;

    if (s == 0) {
        if (debug > 1)
            cprintf("GETTOKEN NULL\n");
        return 0;
    }

    if (debug > 1)
        cprintf("GETTOKEN: %s\n", s);

    *p1 = 0;
    *p2 = 0;

    while (strchr(WHITESPACE, *s))
        *s++ = 0;
    if (*s == 0) {
        if (debug > 1)
            cprintf("EOL\n");
        return 0;
    }
    if (strchr(SYMBOLS, *s)) {
        t = *s;
        *p1 = s;
        *s++ = 0;
        *p2 = s;
        if (debug > 1)
            cprintf("TOK %c\n", t);
        return t;
    }
    *p1 = s;
    while (*s && !strchr(WHITESPACE SYMBOLS, *s))
        s++;
    *p2 = s;
    if (debug > 1) {
        t = **p2;
        **p2 = 0;
        cprintf("WORD: %s\n", *p1);
        **p2 = t;
    }
    return 'w';
}

int
gettoken(char *s, char **p1) {
    static int c, nc;
    static char *np1, *np2;

    if (s) {
        nc = _gettoken(s, &np1, &np2);
        return 0;
    }
    c = nc;
    *p1 = np1;
    nc = _gettoken(np2, &np1, &np2);
    return c;
}

static void
cd_emulation(const char *buf) {
    char full_path[100];
    int res = get_cwd(full_path, 100);
    if (res) {
        printf("cd: %i\r\n", res);
        return;
    }
    char *dir = strchr(buf, ' ');
    if (dir == NULL) {
        return;
    }
    while (*dir == ' ') {
        dir += 1;
    }

    if (strncmp(dir, "..", 2) == 0) {
        char *last_dir = strchr(full_path, '/') + 1;
        while (strchr(last_dir, '/')) {
            last_dir = strchr(last_dir, '/') + 1;
        }
        char new_cwd[100];
        memset(new_cwd, 0, sizeof new_cwd);
        strncpy(new_cwd, full_path, last_dir - full_path);
        res = set_cwd(new_cwd);
        if (res) {
            printf("cd: %s: %i\r\n", strchr(buf, ' ') + 1, res);
        }
        return;
    }

    strcat(full_path, dir);

    res = set_cwd(full_path);
    if (res) {
        printf("cd: %s: %i\r\n", strchr(buf, ' ') + 1, res);
    }
}

static void
get_current_user(char *out_buffer) {
    static union UsersdRequest request = {};
    request.get_env_info.target = sys_getenvid();

    union UsersdResponse *response = (void *)RECEIVE_ADDR;

    int res = rpc_execute(kmod_find_any_version(USERSD_MODNAME), USERSD_REQ_GET_ENV_INFO, &request, (void **)&response);
    if (res < 0) {
        printf("sh: %i\r\n", res);
        return;
    }
    get_username(response->env_info.info.ruid, out_buffer);
    sys_unmap_region(CURENVID, (void *)RECEIVE_ADDR, PAGE_SIZE);
}

void
usage(void) {
    cprintf("usage: sh [-dix] [command-file]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int r, interactive, echocmds, pipe;
    struct Argstate args;

    interactive = '?';
    echocmds = 0;
    pipe = 0;
    argstart(&argc, argv, &args);
    while ((r = argnext(&args)) >= 0) {
        switch (r) {
        case 'd':
            break;
        case 'i':
            interactive = 1;
            break;
        case 'x':
            echocmds = 1;
            break;
        case 'p':
            pipe = 1;
            break;
        default:
            usage();
        }
    }

    if (argc > 2 && !pipe)
        usage();
    if (argc == 3) {
        long stdin, stdout;
        str_to_long(argv[1], 10, &stdin);
        str_to_long(argv[2], 10, &stdout);
        close(0);
        close(1);
        dup(stdin, 0);
        dup(stdout, 1);
        cprintf("success shell configuration\n");
    }
    if (argc == 2) {
        close(0);
        if ((r = open(argv[1], O_RDONLY)) < 0)
            panic("open %s: %i", argv[1], r);
        assert(r == 0);
    }
    if (interactive == '?' && !pipe)
        interactive = iscons(0);

    while (1) {
        sys_yield();
        char *buf;

        char cwd[BUFSIZ];
        memset(cwd, 0, sizeof(cwd));
        strcpy(cwd, "\r"green);
        get_current_user(cwd + strlen(cwd));
        strcat(cwd, clear ":" blue);
        get_cwd(cwd + strlen(cwd), MAXPATHLEN);
        strcat(cwd, clear "$ ");

        buf = readline(interactive ? cwd : NULL);
        if (buf == NULL) {
            if (debug) cprintf("EXITING\n");
            exit(); /* end of file */
        }

        if (strncmp(buf, "cd", 2) == 0) {
            cd_emulation(buf);
            continue;
        }

        if (strncmp(buf, "exit", 4) == 0) exit();
        if (debug) cprintf("LINE: %s\n", buf);
        if (buf[0] == '#') continue;
        if (echocmds) printf("# %s\n", buf);
        if (debug) cprintf("BEFORE FORK\n");
        if ((r = fork()) < 0) {
            printf("sh: fork: %x", r);
            continue;
        }
        if (debug) cprintf("FORK: %x\n", r);
        if (r == 0) {
            runcmd(buf);
            exit();
        } else
            wait(r);
    }
}
