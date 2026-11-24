#ifndef _DOOM_MATH_H
#define _DOOM_MATH_H
static inline double fabs(double x) { return x < 0 ? -x : x; }
static inline float fabsf(float x) { return x < 0 ? -x : x; }
double sqrt(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double atan2(double y, double x);
double floor(double x);
double ceil(double x);
double pow(double x, double y);
#endif
