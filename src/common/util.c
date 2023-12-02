#include "util.h"

void memcpy(void* dst, void* src, int len) {
    for(int i = 0; i < len; i++) {
        ((char*) dst)[i] = ((char*)src)[i];
    }
}

void memset(void* dst, char value, int len) {
    for(int i = 0; i < len; i++) {
        ((char*) dst)[i] = value;
    }
}

int abs(int value) {
    if(value < 0) return -value;
    else return value;
}