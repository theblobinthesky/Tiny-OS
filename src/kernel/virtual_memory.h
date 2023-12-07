#pragma once
#include "util.h"

#define VM_PAGE_SIZE 4096

void vm_init(void* memory_map, u64 memory_map_size, u64 memory_map_desc_size);
void *vm_palloc(u64 num_pages);
void vm_pfree(void *v_address, u64 num_pages);