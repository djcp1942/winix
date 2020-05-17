#include <sys/syscall.h>
#include <errno.h>
#include <stddef.h>

#define ESTR_SIZ    (32)

static char estr[ESTR_SIZ];

int __dprintf(int fd, const char *format, void* args){
    struct message m;
    m.m1_i1 = fd;
    m.m1_p1 = (void *)format;
    m.m1_p2 = args;
    return _syscall(WINIX_DPRINTF, &m);
}

int dprintf(int fd, const char *format, ...){
    
    return __dprintf(fd, format, (int*)&format + 1);
}

int printf(const char *format, ...) {
    return __dprintf(1, format, (int*)&format + 1);
}

int __strerror(char *buffer, int len){
    struct message m;
    m.m1_p1 = buffer;
    m.m1_i1 = len;
    m.m1_i2 = errno;
    return _syscall(WINIX_STRERROR, &m);
}

char* strerror(int usrerrno){
    __strerror(estr, ESTR_SIZ);
    return estr;
}

void perror(const char *s){
    __strerror(estr, ESTR_SIZ);
    dprintf(2, "%s: %s\n", s, estr);
}
