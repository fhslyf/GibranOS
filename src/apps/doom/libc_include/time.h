#ifndef _DOOM_TIME_H
#define _DOOM_TIME_H
typedef long time_t;
struct tm { int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year; };
time_t time(time_t* tloc);
#endif
