#include "cpuid.h"

struct {
    char vendor_id[13];
    u32 max_calling_param;
    u32 signature;
    u32 capabilities;
    u32 extended_capabilities;
} cpuid_data;

void cpuid_init() {
    u32 id[3] = {0};
    asm volatile("cpuid" : "=eax"(cpuid_data.max_calling_param), "=ebx"(id[0]), "=edx"(id[1]), "=ecx"(id[2]) : "eax"(0x0));
    memcpy(&cpuid_data.vendor_id[0], &id[0], 12);

    u64 cap_1, cap_2;
    asm volatile("cpuid" : "=eax"(cpuid_data.signature), "=edx"(cap_1), "=ecx"(cap_2) : "eax"(0x1) : "%rbx");
    cpuid_data.capabilities = (cap_2 << 32) | cap_1;

    u32 ext_cap1, ext_cap2;
    asm volatile("cpuid" : "=ebx"(ext_cap1), "=ecx"(ext_cap2) : "eax"(0x7), "ecx"(0x0) : "%rax", "%rdx");
    cpuid_data.extended_capabilities = ((u64)ext_cap2 << 32) | (u64)ext_cap1;
}

bool cpuid_has_capability(enum cpu_capability capability) {
    return (cpuid_data.capabilities & (1ULL << capability)) != 0;
}

bool cpuid_has_extended_capability(enum cpu_extended_capability capability) {
    return (cpuid_data.extended_capabilities & (1ULL << capability)) != 0;
}