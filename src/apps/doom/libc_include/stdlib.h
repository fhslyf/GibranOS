#ifndef _DOOM_STDLIB_H
#define _DOOM_STDLIB_H
#include <stddef.h>

#define RAND_MAX 32767
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void exit(int status);
void abort(void);
int atoi(const char* nptr);
long atol(const char* nptr);
double atof(const char* nptr);
long strtol(const char* nptr, char** endptr, int base);
unsigned long strtoul(const char* nptr, char** endptr, int base);
int abs(int j);
long labs(long j);
void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));
char* getenv(const char* name);
int system(const char* command);
int rand(void);
void srand(unsigned int seed);
#endif
