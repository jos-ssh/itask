#pragma once

#define TIOCSWINSZ	0x5414

int ioctl(int fd, unsigned long request, ...);
