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

int memcmp(const void *ptr1, const void *ptr2, u32 num) {
    const u8* uptr1 = (u8 *)ptr1;
    const u8* uptr2 = (u8 *)ptr2;

    for (u32 i = 0; i < num; i++) {
        if (uptr1[i] < uptr2[i]) {
            return -1;
        } else if (uptr1[i] > uptr2[i]) {
            return 1;
        }
    }

    return 0;
}

int abs(int value) {
    if(value < 0) return -value;
    else return value;
}