#ifndef _DOOM_SYS_STAT_H
#define _DOOM_SYS_STAT_H
#include "types.h"
struct stat { off_t st_size; mode_t st_mode; };
int stat(const char* path, struct stat* buf);
int mkdir(const char* path, mode_t mode);
#endif
