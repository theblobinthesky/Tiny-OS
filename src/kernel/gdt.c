#include "gdt.h"

#define GDT_ENTRY_TYPE_CODE 1
#define GDT_ENTRY_TYPE_DATA 2
#define GDT_ENTRY_TYPE_TSS 3
#define GDT_ENTRY_COUNT 3

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8 base_middle;

    // access byte
    u8 accessed_bit : 1;
    u8 rw_bit : 1;
    u8 dir_bit : 1;
    u8 exec_bit : 1;
    u8 desc_type_bit : 1;
    u8 dpl : 2;
    u8 present_bit : 1;

    u8 limit_high : 4;
    
    // flags nibble
    u8 reserved_bit : 1;
    u8 is_64_bit : 1;
    u8 is_32_bit : 1;
    u8 granularity_flag : 1;

    u8 base_high;
} __attribute__((packed));

struct gdt_entry gdt[GDT_ENTRY_COUNT];

void get_gdt_entry(u32 index, u8 entry_type, bool is_kernel_mode) {
    struct gdt_entry* entry = gdt + (index / sizeof(struct gdt_entry));
    memset(entry, 0, sizeof(struct gdt_entry));

    // Base, limit and granularity are ignored in 64 bit mode. 
    // The segment just spans the entire address space.

    // Access byte:
    entry->accessed_bit = 1;
    entry->rw_bit = 1; // this might be suboptimal in the future
    entry->dir_bit = 0; // segment grows towards larger addresses
    entry->exec_bit = (entry_type == GDT_ENTRY_TYPE_CODE) ? 1 : 0;
    entry->desc_type_bit = (entry_type == GDT_ENTRY_TYPE_TSS) ? 0 : 1;
    entry->dpl = is_kernel_mode ? 0 : 3;
    entry->present_bit = 1;
    
    // Flags nibble:
    entry->is_64_bit = (entry_type == GDT_ENTRY_TYPE_CODE);
}

void gdt_init() {
    memset(gdt + 0, 0, sizeof(struct gdt_entry));
    get_gdt_entry(KERNEL_CODE_SEGMENT_DESC, GDT_ENTRY_TYPE_CODE, true);
    get_gdt_entry(KERNEL_DATA_SEGMENT_DESC, GDT_ENTRY_TYPE_DATA, true);
    // get_gdt_entry(USER_CODE_SEGMENT_DESC, GDT_ENTRY_TYPE_CODE, false);
    // get_gdt_entry(USER_DATA_SEGMENT_DESC, GDT_ENTRY_TYPE_DATA, false);

    lgdt(gdt, sizeof(gdt));
}