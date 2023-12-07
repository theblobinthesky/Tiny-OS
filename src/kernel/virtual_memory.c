#include "virtual_memory.h"
#include "linked_list.h"
#include <efi.h>
#include <efilib.h>

struct {
    u64 num_of_free_pages;
    linked_list free_list;
} vm_data = {};

typedef struct {
    linked_list_entry list_entry;
    u64 num_pages;
} free_block_entry;

static u64 free_block_end(free_block_entry *entry) {
    return (u64)entry + entry->num_pages * VM_PAGE_SIZE;
}

static void add_to_free_list(u64 p_start, u64 num_pages) {
    // The kernel memory is identity mapped so we can use virtual memory and physical 
    // memory addresses interchangably. We assume there cannot be intersections of 
    // free block entries since free memory can only be added once.

    free_block_entry *block = (free_block_entry *)p_start;
    block->num_pages = num_pages;
        
    free_block_entry *entry = (free_block_entry *)linked_list_head(&vm_data.free_list);
    while (entry != NULL && p_start > free_block_end(entry)) {
        entry = (free_block_entry *)linked_list_next(&entry->list_entry);
    }

    if (entry == NULL) {
        // If this is the last element and there is no intersection 
        // simply insert it at the back.
        linked_list_add(&vm_data.free_list, &block->list_entry);
    } else if (free_block_end(block) < (u64)entry) {
        // If the block is in the middle and there is no intersection
        // simply insert it before the entry.
        linked_list_insert_before(&entry->list_entry, &block->list_entry);
    } else {
        // If the block intersects with its successor at the end
        // simply keep merging until you're done.
        while (entry != null && free_block_end(block) == (u64)entry) {
            block->num_pages += entry->num_pages;
            free_block_entry *tmp = entry;
            entry = (free_block_entry *)linked_list_next(&entry->list_entry);
            linked_list_remove(&tmp->list_entry);
        }
        
        linked_list_add(&vm_data.free_list, &block->list_entry);
    }
}

void vm_init(void* memory_map, u64 memory_map_size, u64 memory_map_desc_size) {
    int num_of_memory_descs = memory_map_size / memory_map_desc_size;

    for (int i = 0; i < num_of_memory_descs; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (memory_map + i * memory_map_desc_size);
        
        // The memory map solves the problem of not-knowing where memory mapped i/o
        // regions reside in BIOS systems. At least it sure seems like it.
        // Also we can't use pages within the first megabyte of memory, since
        // they might be used by all kinds of legacy devices like vga video memory.
        // Note: The kernel was allocated within EfiLoaderData.
        int type = desc->Type;
        if (desc->PhysicalStart >= 0x100000 && (type == EfiLoaderCode 
            || type == EfiBootServicesData || type == EfiConventionalMemory)) {
            add_to_free_list((u64) desc->PhysicalStart, desc->NumberOfPages);
            vm_data.num_of_free_pages += desc->NumberOfPages;
        }
    }
}

void *vm_palloc(u64 num_pages) {
    free_block_entry *free_block = (free_block_entry *)linked_list_head(&vm_data.free_list);

    while (free_block != null && free_block->num_pages < num_pages) {
        free_block = (free_block_entry *)linked_list_next(&free_block->list_entry);
    }

    if (free_block == null) {
        return null;
    } 
    
    void *pages = (void *)free_block + (free_block->num_pages - num_pages) * VM_PAGE_SIZE;
    free_block->num_pages -= num_pages;

    if (free_block->num_pages == 0) {
        linked_list_remove(&free_block->list_entry);
    }

    memset(pages, 0, num_pages * VM_PAGE_SIZE);

    return pages;
}

void vm_pfree(void *v_address, u64 num_pages) {
    add_to_free_list((u64) v_address, num_pages);
}