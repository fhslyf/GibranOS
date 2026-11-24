#ifndef _DOOM_UNISTD_H
#define _DOOM_UNISTD_H
#include <sys/types.h>
int access(const char* pathname, int mode);
int unlink(const char* pathname);
#endif
