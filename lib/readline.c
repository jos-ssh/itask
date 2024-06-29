#include "inc/lib.h"
#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/types.h>
#include <inc/string.h>

#define BUFLEN 1024

static char buf[BUFLEN];

static char *
readline_impl(const char *prompt, bool is_echo) {
    if (prompt) {
#if JOS_KERNEL
        cprintf("%s", prompt);
#else
        fprintf(1, "%s", prompt);
#endif
    }

    bool echo = is_echo;

    for (size_t i = 0;;) {
#ifdef JOS_KERNEL
        int c = getchar();
#else
        int c;
        int res = read(0, &c, 1);
        sys_yield();
        if (res == 0 || res == -E_WOULD_BLOCK) continue;
        if (res < 0) c = res;
#endif

        if (c < 0) {
            if (c != -E_EOF)
                cprintf("read error: %i\n", c);
            return NULL;
        } else if ((c == '\b' || c == '\x7F')) {
            if (i) {
                if (echo) {
#if JOS_KERNEL
                    cputchar('\b');
                    cputchar(' ');
                    cputchar('\b');
#else
                    fprintf(1, "\b \b");
#endif
                }
                i--;
            }
        } else if (c >= ' ') {
            if (i < BUFLEN - 1) {
                if (echo) {
#if JOS_KERNEL
                    cputchar(c);
#else
                    fprintf(1, "%c", c);
#endif
                }
                buf[i++] = (char)c;
            }
            // FIXME: it seem's that when we reach end of buffer, we start to just ignore chars
        } else if (c == '\n' || c == '\r') {
            if (echo) {
#if JOS_KERNEL
                cputchar('\n');
#else
                fprintf(1, "\r\n");
#endif
            }
            buf[i] = 0;
            return buf;
        }
    }
}

char *
readline(const char *prompt) {
    return readline_impl(prompt, true);
}

char *
readline_noecho(const char *prompt) {
    return readline_impl(prompt, false);
}
