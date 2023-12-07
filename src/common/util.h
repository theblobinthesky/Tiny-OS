#pragma once

#define bool int
#define true 1
#define false 0
typedef __INT8_TYPE__ s8;
typedef __UINT8_TYPE__ u8;
typedef __INT16_TYPE__ s16;
typedef __UINT16_TYPE__ u16;
typedef __INT32_TYPE__ s32;
typedef __UINT32_TYPE__ u32;
typedef __INT64_TYPE__ s64;
typedef __UINT64_TYPE__ u64;
typedef float f32;
typedef double f64;
#define null ((void *)0)

void memcpy(void* dst, void* src, int len);
void memset(void* dst, char value, int len);
int abs(int value);