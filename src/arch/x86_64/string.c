#include <stddef.h>
#include "string.h"

extern void *memset(void *dst, int c, size_t n){
    char *dst_c = (char *) dst;
    for(int i = 0; i < n; i++){
        dst_c[i] = c;
    }
    return dst;
}

extern void *memcpy(void *dst, const void *src, size_t n){
    char *src_c = (char *)src;
    char *dst_c = (char *)dst;
    for(int i = 0; i < n; i++){
        dst_c[i] = src_c[i];
    }
    return dst;
}