
#ifndef _UTIL_H
#define _UTIL_H

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static int64_t millis() {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

static void subst(char *s, char from, char to) {
    while (*s == from)
        *s++ = to;
}

#endif
