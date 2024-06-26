#include <inc/syslog.h>
#include <inc/lib.h>

void openlog(const char *ident, int option, int facility) NOTIMPLEMENTED(void);
void syslog(int priority, const char *format, ...) NOTIMPLEMENTED(void);
void closelog(void) NOTIMPLEMENTED(void);