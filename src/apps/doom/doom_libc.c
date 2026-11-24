#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "doom1_wad.h"

extern void* kmalloc(size_t size);
extern void  kfree(void* ptr);

#undef errno
int errno = 0;

void* malloc(size_t size) { return kmalloc(size); }
void  free(void* ptr)     { if (ptr) kfree(ptr); }

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = kmalloc(total);
    if (ptr) {
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < total; i++) p[i] = 0;
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        uint8_t* dst = (uint8_t*)new_ptr;
        uint8_t* src = (uint8_t*)ptr;
        for (size_t i = 0; i < size; i++) dst[i] = src[i];
        kfree(ptr);
    }
    return new_ptr;
}

// --- Fungsi Memori & String ---
__attribute__((weak)) void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

__attribute__((weak)) void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else if (d > s) {
        for (size_t i = n; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dest;
}

__attribute__((weak)) void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

__attribute__((weak)) int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* a = (const uint8_t*)s1;
    const uint8_t* b = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

__attribute__((weak)) void* memchr(const void* s, int c, size_t n) {
    const uint8_t* p = (const uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        if (p[i] == (uint8_t)c) return (void*)(p + i);
    }
    return NULL;
}

__attribute__((weak)) size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

__attribute__((weak)) char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

__attribute__((weak)) char* strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

__attribute__((weak)) char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

__attribute__((weak)) char* strncat(char* dest, const char* src, size_t n) {
    char* d = dest + strlen(dest);
    size_t i = 0;
    for (; i < n && src[i]; i++) d[i] = src[i];
    d[i] = '\0';
    return dest;
}

__attribute__((weak)) int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

__attribute__((weak)) int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || !s1[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

static int to_lower_ch(int c) { return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; }

int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && (to_lower_ch(*s1) == to_lower_ch(*s2))) { s1++; s2++; }
    return to_lower_ch(*(const unsigned char*)s1) - to_lower_ch(*(const unsigned char*)s2);
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int c1 = to_lower_ch((unsigned char)s1[i]);
        int c2 = to_lower_ch((unsigned char)s2[i]);
        if (c1 != c2 || !s1[i]) return c1 - c2;
    }
    return 0;
}

__attribute__((weak)) char* strchr(const char* s, int c) {
    while (*s) { if (*s == (char)c) return (char*)s; s++; }
    return (c == 0) ? (char*)s : NULL;
}

__attribute__((weak)) char* strrchr(const char* s, int c) {
    const char* last = NULL;
    do { if (*s == (char)c) last = s; } while (*s++);
    return (char*)last;
}

__attribute__((weak)) char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* copy = (char*)kmalloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

char* strerror(int errnum) { (void)errnum; return "err"; }

// --- Konversi Angka & Utilitas ---
int abs(int j) { return j < 0 ? -j : j; }
long labs(long j) { return j < 0 ? -j : j; }

long strtol(const char* nptr, char** endptr, int base) {
    while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') nptr++;
    int neg = 0;
    if (*nptr == '-') { neg = 1; nptr++; }
    else if (*nptr == '+') { nptr++; }
    if (base == 0) {
        if (nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) { base = 16; nptr += 2; }
        else if (nptr[0] == '0') { base = 8; }
        else { base = 10; }
    } else if (base == 16 && nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X')) {
        nptr += 2;
    }
    long val = 0;
    while (*nptr) {
        int d = -1;
        if (*nptr >= '0' && *nptr <= '9') d = *nptr - '0';
        else if (*nptr >= 'a' && *nptr <= 'z') d = *nptr - 'a' + 10;
        else if (*nptr >= 'A' && *nptr <= 'Z') d = *nptr - 'A' + 10;
        if (d < 0 || d >= base) break;
        val = val * base + d;
        nptr++;
    }
    if (endptr) *endptr = (char*)nptr;
    return neg ? -val : val;
}

unsigned long strtoul(const char* nptr, char** endptr, int base) {
    return (unsigned long)strtol(nptr, endptr, base);
}

int atoi(const char* nptr) { return (int)strtol(nptr, NULL, 10); }
long atol(const char* nptr) { return strtol(nptr, NULL, 10); }
double atof(const char* nptr) { return (double)strtol(nptr, NULL, 10); }

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*)) {
    uint8_t* arr = (uint8_t*)base;
    uint8_t tmp[256];
    if (size > sizeof(tmp)) return;
    for (size_t i = 1; i < nmemb; i++) {
        memcpy(tmp, arr + i * size, size);
        size_t j = i;
        while (j > 0 && compar(arr + (j - 1) * size, tmp) > 0) {
            memcpy(arr + j * size, arr + (j - 1) * size, size);
            j--;
        }
        memcpy(arr + j * size, tmp, size);
    }
}

void abort(void) { extern void exit(int); exit(1); }
char* getenv(const char* name) { (void)name; return NULL; }
int system(const char* cmd) { (void)cmd; return -1; }
static unsigned int rand_seed = 1;
int rand(void) { rand_seed = rand_seed * 1103515245 + 12345; return (int)((rand_seed >> 16) & 0x7FFF); }
void srand(unsigned int seed) { rand_seed = seed; }

// --- Virtual File Wrapper Multi-Handle untuk DOOM1.WAD ---
typedef struct {
    int in_use;
    size_t pos;
    size_t size;
    const uint8_t* data;
} MEM_FILE;

static MEM_FILE wad_handles[8];

static int is_doom1_wad(const char* s) {
    if (!s) return 0;
    const char* base = s;
    for (const char* p = s; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    return strcasecmp(base, "doom1.wad") == 0;
}

void* fopen(const char* filename, const char* mode) {
    if (!filename || !mode) return NULL;
    if (mode[0] == 'r' && is_doom1_wad(filename)) {
        for (int i = 0; i < 8; i++) {
            if (!wad_handles[i].in_use) {
                wad_handles[i].in_use = 1;
                wad_handles[i].pos    = 0;
                wad_handles[i].size   = DOOM1_WAD_len;
                wad_handles[i].data   = DOOM1_WAD;
                return &wad_handles[i];
            }
        }
    }
    return NULL;
}

size_t fread(void* ptr, size_t size, size_t nmemb, void* stream) {
    if (!stream || size == 0) return 0;
    MEM_FILE* f = (MEM_FILE*)stream;
    size_t bytes = size * nmemb;
    if (f->pos + bytes > f->size) bytes = f->size - f->pos;
    memcpy(ptr, f->data + f->pos, bytes);
    f->pos += bytes;
    return bytes / size;
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, void* stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    return 0;
}

int fseek(void* stream, long offset, int whence) {
    if (!stream) return -1;
    MEM_FILE* f = (MEM_FILE*)stream;
    if (whence == 0)      f->pos = offset;
    else if (whence == 1) f->pos += offset;
    else if (whence == 2) f->pos = f->size + offset;
    return 0;
}

long ftell(void* stream) { return stream ? (long)((MEM_FILE*)stream)->pos : -1; }
int  fclose(void* stream) { if (stream) ((MEM_FILE*)stream)->in_use = 0; return 0; }
int  feof(void* stream)   { return stream ? (((MEM_FILE*)stream)->pos >= ((MEM_FILE*)stream)->size) : 1; }
int  fflush(void* stream) { (void)stream; return 0; }
int  ferror(void* stream) { (void)stream; return 0; }
int  remove(const char* f) { (void)f; return -1; }
int  rename(const char* o, const char* n) { (void)o; (void)n; return -1; }
int  stat(const char* p, void* b) { (void)p; (void)b; return -1; }
int  mkdir(const char* p, int m) { (void)p; (void)m; return 0; }
int  access(const char* p, int m) { (void)m; return is_doom1_wad(p) ? 0 : -1; }
int  unlink(const char* p) { (void)p; return -1; }
long time(long* t) { if (t) *t = 0; return 0; }

// --- Formatter String & Output Serial Port (COM1) ---
static void put_ch(char* buf, size_t max, size_t* idx, char c) {
    if (buf && *idx + 1 < max) buf[*idx] = c;
    (*idx)++;
}

static void put_num(char* buf, size_t max, size_t* idx, unsigned long val, int base, int neg, int width, int prec, char pad) {
    char tmp[32];
    int len = 0;
    const char* digits = "0123456789abcdef";
    if (val == 0) tmp[len++] = '0';
    while (val > 0) { tmp[len++] = digits[val % base]; val /= base; }
    while (len < prec) tmp[len++] = '0';
    int total = len + (neg ? 1 : 0);
    while (total < width) { put_ch(buf, max, idx, pad); total++; }
    if (neg) put_ch(buf, max, idx, '-');
    while (len > 0) put_ch(buf, max, idx, tmp[--len]);
}

int vsnprintf(char* str, size_t size, const char* fmt, va_list ap) {
    size_t idx = 0;
    while (*fmt) {
        if (*fmt != '%') { put_ch(str, size, &idx, *fmt++); continue; }
        fmt++;
        int left = 0;
        if (*fmt == '-') { left = 1; fmt++; }
        char pad = (*fmt == '0' && !left) ? '0' : ' ';
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        int prec = -1;
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            while (*fmt >= '0' && *fmt <= '9') { prec = prec * 10 + (*fmt++ - '0'); }
        }
        if (*fmt == 'l') fmt++;
        switch (*fmt) {
            case 'd': case 'i': {
                long v = va_arg(ap, int);
                int neg = v < 0;
                put_num(str, size, &idx, neg ? (unsigned long)(-v) : (unsigned long)v, 10, neg, width, prec, pad);
                break;
            }
            case 'u': put_num(str, size, &idx, va_arg(ap, unsigned int), 10, 0, width, prec, pad); break;
            case 'x': case 'X': case 'p': put_num(str, size, &idx, va_arg(ap, unsigned int), 16, 0, width, prec, pad); break;
            case 'c': put_ch(str, size, &idx, (char)va_arg(ap, int)); break;
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                int slen = 0;
                while (s[slen] && (prec < 0 || slen < prec)) slen++;
                if (!left) { while (slen < width) { put_ch(str, size, &idx, ' '); width--; } }
                for (int i = 0; i < slen; i++) put_ch(str, size, &idx, s[i]);
                if (left) { while (slen < width) { put_ch(str, size, &idx, ' '); width--; } }
                break;
            }
            case '%': put_ch(str, size, &idx, '%'); break;
            default: break;
        }
        if (*fmt) fmt++;
    }
    if (str && size > 0) str[idx < size ? idx : size - 1] = '\0';
    return (int)idx;
}
int snprintf(char* str, size_t size, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(str, size, fmt, ap);
    va_end(ap); return r;
}

int sprintf(char* str, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(str, 0x7FFFFFFF, fmt, ap);
    va_end(ap); return r;
}

static void doom_serial_putc(char c) {
    uint8_t status;
    do {
        __asm__ volatile("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
    } while ((status & 0x20) == 0);
    __asm__ volatile("outb %0, %1" :: "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
}

int vfprintf(void* s, const char* fmt, va_list ap) {
    (void)s;
    char buf[512];
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    for (int i = 0; i < len && buf[i]; i++) {
        if (buf[i] == '\n') doom_serial_putc('\r');
        doom_serial_putc(buf[i]);
    }
    return len;
}

int printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vfprintf(NULL, fmt, ap);
    va_end(ap); return r;
}

int fprintf(void* s, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vfprintf(s, fmt, ap);
    va_end(ap); return r;
}

int sscanf(const char* s, const char* fmt, ...) { (void)s; (void)fmt; return 0; }
int putchar(int c) { doom_serial_putc((char)c); return c; }
int puts(const char* s) { while (*s) doom_serial_putc(*s++); doom_serial_putc('\n'); return 0; }

// --- Fungsi Matematika Dasar ---
double sqrt(double x) {
    if (x <= 0) return 0;
    double r = x;
    for (int i = 0; i < 16; i++) r = 0.5 * (r + x / r);
    return r;
}
double floor(double x) { long i = (long)x; return (x < i) ? (i - 1) : i; }
double ceil(double x)  { long i = (long)x; return (x > i) ? (i + 1) : i; }
double sin(double x)   { (void)x; return 0.0; }
double cos(double x)   { (void)x; return 1.0; }
double tan(double x)   { (void)x; return 0.0; }
double atan2(double y, double x) { (void)y; (void)x; return 0.0; }
double pow(double x, double y) { (void)x; (void)y; return 1.0; }

// --- 64-bit Integer Division untuk arsitektur 32-bit i386 (-nostdlib) ---
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t* rem_p) {
    uint64_t quot = 0, qbit = 1;
    if (den == 0) return 0;
    while ((int64_t)den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }
    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }
    if (rem_p) *rem_p = num;
    return quot;
}

uint64_t __udivdi3(uint64_t num, uint64_t den) {
    return __udivmoddi4(num, den, NULL);
}

uint64_t __umoddi3(uint64_t num, uint64_t den) {
    uint64_t rem = 0;
    __udivmoddi4(num, den, &rem);
    return rem;
}

int64_t __divdi3(int64_t num, int64_t den) {
    int neg = 0;
    uint64_t unum = (num < 0) ? (neg = !neg, -(uint64_t)num) : (uint64_t)num;
    uint64_t uden = (den < 0) ? (neg = !neg, -(uint64_t)den) : (uint64_t)den;
    uint64_t res = __udivmoddi4(unum, uden, NULL);
    return neg ? -(int64_t)res : (int64_t)res;
}

int64_t __moddi3(int64_t num, int64_t den) {
    int neg = 0;
    uint64_t unum = (num < 0) ? (neg = 1, -(uint64_t)num) : (uint64_t)num;
    uint64_t uden = (den < 0) ? -(uint64_t)den : (uint64_t)den;
    uint64_t rem = 0;
    __udivmoddi4(unum, uden, &rem);
    return neg ? -(int64_t)rem : (int64_t)rem;
}
