#ifndef _DOOM_STDIO_H
#define _DOOM_STDIO_H
#include <stddef.h>
#include <stdarg.h>

typedef void FILE;
#define stdin  ((FILE*)0)
#define stdout ((FILE*)1)
#define stderr ((FILE*)2)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define EOF (-1)
#define FILENAME_MAX 260

FILE* fopen(const char* filename, const char* mode);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int fclose(FILE* stream);
int feof(FILE* stream);
int fflush(FILE* stream);
int ferror(FILE* stream);
int remove(const char* filename);
int rename(const char* oldname, const char* newname);
int printf(const char* fmt, ...);
int fprintf(FILE* stream, const char* fmt, ...);
int sprintf(char* str, const char* fmt, ...);
int snprintf(char* str, size_t size, const char* fmt, ...);
int vsnprintf(char* str, size_t size, const char* fmt, va_list ap);
int vfprintf(FILE* stream, const char* fmt, va_list ap);
int sscanf(const char* str, const char* fmt, ...);
int putchar(int c);
int puts(const char* s);
#endif
