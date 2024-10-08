#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>
#include <stddef.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define EOF -1

/* lib/console.c*/
void cputchar(int c);
int getchar(void);
int iscons(int fd);

/* lib/stdio.c */
struct Fd;

struct Fd* fopen(const char *path, int mode);
int fclose(struct Fd* stream);

int fgetc(struct Fd* stream);
char *fgets(char *s, int size, struct Fd* stream);


/* lib/printfmt.c */
void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list) __attribute__((format(printf, 3, 0)));
int snprintf(char *str, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char *str, size_t size, const char *fmt, va_list) __attribute__((format(printf, 3, 0)));

/* lib/printf.c */
int cprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vcprintf(const char *fmt, va_list) __attribute__((format(printf, 1, 0)));

/* lib/fprintf.c */
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int fprintf(int fd, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(int fd, const char *fmt, va_list);

/* lib/readline.c */
char *readline(const char *prompt);
char *readline_noecho(const char *promtt);


#endif /* !JOS_INC_STDIO_H */
