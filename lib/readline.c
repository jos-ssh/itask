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

    bool echo = iscons(0) && is_echo;

    for (size_t i = 0;;) {
        int c = getchar();

        if (c < 0) {
            if (c != -E_EOF)
                cprintf("read error: %i\n", c);
            return NULL;
        } else if ((c == '\b' || c == '\x7F')) {
            if (i) {
                if (echo) {
                    cputchar('\b');
                    cputchar(' ');
                    cputchar('\b');
                }
                i--;
            }
        } else if (c >= ' ') {
            if (i < BUFLEN - 1) {
                if (echo) {
                    cputchar(c);
                }
                buf[i++] = (char)c;
            }
            // FIXME: it seem's that when we reach end of buffer, we start to just ignore chars
        } else if (c == '\n' || c == '\r') {
            if (echo) {
                cputchar('\n');
            }
            buf[i] = 0;
            return buf;
        }
    }
}

char *readline(const char *prompt) {
    return readline_impl(prompt, true);
}

char *readline_noecho(const char *prompt) {
    return readline_impl(prompt, false);
}
