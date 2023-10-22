#include "kernel_loader.h"
#include "efi_main.h"
#include "elf.h"
#include "util.h"

static bool verify_elf_ident(Elf64_Ehdr* header) {
    bool is_magic_number = (header->e_ident[0] == 0x7F) &&
        (header->e_ident[1] == 'E') &&
        (header->e_ident[2] == 'L') && 
        (header->e_ident[3] == 'F');
    bool is_64_bit = (header->e_ident[4] == ELFCLASS64);
    bool is_little_endian = (header->e_ident[5] == ELFDATA2LSB);
    bool is_systemv_os_abi = (header->e_ident[7] == ELFOSABI_NONE);

    if(!(is_magic_number && is_64_bit && is_little_endian && is_systemv_os_abi)) return false;

    bool is_executable = (header->e_type == ET_EXEC);
    bool is_x86_64 = (header->e_machine == EM_X86_64);
    bool is_current_version = (header->e_version == EV_CURRENT);

    if(!is_executable || !is_x86_64 || !is_current_version) return false;

    return true;
}

EFI_STATUS load_kernel(EFI_HANDLE image_handle) {
    EFI_STATUS status;

    // load the image protocol of this image; an image is essentially the executable PE file with elf subsystem generated by the linker
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image = NULL;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = BS->HandleProtocol(image_handle, &loaded_image_guid, (VOID**)&loaded_image);
    ASSERT_NO_EFI_ERROR();

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* file_system = NULL;
    EFI_GUID file_system_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = BS->HandleProtocol(loaded_image->DeviceHandle, &file_system_guid, (VOID**)&file_system);
    ASSERT_NO_EFI_ERROR();
	
    EFI_FILE_HANDLE root_dir;
    status = file_system->OpenVolume(file_system, &root_dir);
    ASSERT_NO_EFI_ERROR();

    EFI_FILE_HANDLE file;
    status = fopen(root_dir, L"KERNEL", EFI_FILE_MODE_READ, 0, &file);
    ASSERT_NO_EFI_ERROR();

    UINT64 kernel_size;
    status = fsize(file, &kernel_size);
    ASSERT_NO_EFI_ERROR();

    VOID* kernel_buffer;
    status = fread(file, kernel_size, &kernel_buffer);
    ASSERT_NO_EFI_ERROR();

    Elf64_Ehdr* elf_header = (Elf64_Ehdr*) kernel_buffer;
    if(!verify_elf_ident(elf_header)) {
        print_error(L"Kernel elf header verification failed.\n");
        return EFI_ABORTED;
    }
    
    Elf64_Phdr* elf_program_headers = (Elf64_Phdr*)(kernel_buffer + elf_header->e_phoff);
    
    // calculate size of kernel in memory
    Elf64_Addr min_virtual_address = 0xffffffffffffffff;
    Elf64_Addr max_virtual_address = 0x0;
    for(int i = 0; i< elf_header->e_phnum; i++) {
        Elf64_Phdr* ph = elf_program_headers + i;

        if(ph->p_flags & PT_LOAD) {
            Elf64_Addr section_end = ph->p_vaddr + ph->p_memsz;
            
            if(ph->p_vaddr < min_virtual_address) min_virtual_address = ph->p_vaddr;
            if(section_end > max_virtual_address) max_virtual_address = section_end;
        }
    }

    int num_bytes = (int)(max_virtual_address - min_virtual_address);
    int num_pages = (num_bytes / EFI_PAGE_SIZE) + 1; // there might be one more page than necessary

    void* kernel_ptr;
    status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, num_pages, (EFI_PHYSICAL_ADDRESS*)&kernel_ptr);
    ASSERT_NO_EFI_ERROR();

    for(int i = 0; i < elf_header->e_phnum; i++) {
        Elf64_Phdr* ph = elf_program_headers + i;

        if(ph->p_flags & PT_LOAD) {
            memcpy((void*)ph->p_vaddr, kernel_ptr + ph->p_offset, ph->p_filesz);
            memset((void*)(ph->p_vaddr + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
        }
    }

    return EFI_SUCCESS;
}