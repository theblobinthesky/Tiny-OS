#pragma once
#include "util.h"

enum cpu_capability {
  CPUID_CAP_TSC = 1ull << 4,
  CPUID_CAP_APIC = 1ull << 9,
  CPUID_CAP_SYSENTER = 1ull << 11,
  CPUID_CAP_XSAVE = 1ull << (32 + 26),
  CPUID_CAP_RDRAND = 1ull << (32 + 30),
};

enum cpu_extended_capability {
  CPUID_EXCAP_RDSEED = 1ull << 18,
};

void cpuid_init();
bool cpuid_has_capability(enum cpu_capability capability);
bool cpuid_has_extended_capability(enum cpu_extended_capability capability);