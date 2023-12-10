#include "../common/common.c"
#include <efi.h>
#include <efilib.h>
#include "kernel_loader.h"
#include "efiConsoleControl.h"

#define DESIRED_FB_RESOLUTION_X 1280
#define DESIRED_FB_RESOLUTION_Y 720

void print_error(CHAR16* str) {
    ST->ConOut->OutputString(ST->ConOut, L"ERROR: ");
    ST->ConOut->OutputString(ST->ConOut, str);
    ST->ConOut->OutputString(ST->ConOut, L"\n");
}

EFI_STATUS fopen(EFI_FILE_HANDLE root_dir, CHAR16* file_name, UINT64 open_mode, UINT64 attributes, EFI_FILE_HANDLE* file) {
    EFI_STATUS status = root_dir->Open(root_dir, file, file_name, open_mode, attributes);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"Efi program failed to open a file.");
        return EFI_ABORTED;
    }
}

EFI_STATUS fsize(EFI_FILE_HANDLE file, UINT64* file_size) {
    EFI_FILE_INFO file_info_buffer[2];
    UINTN file_info_buffer_size = sizeof(file_info_buffer);
    EFI_FILE_INFO* file_info = file_info_buffer;

    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    EFI_STATUS status = file->GetInfo(file, &file_info_guid, &file_info_buffer_size, (VOID*) &file_info_buffer);

    if(status == EFI_SUCCESS) {
        *file_size = file_info->FileSize;
        return EFI_SUCCESS;
    } else {
        *file_size = 0;
        print_error(L"Efi program failed to get the size of a file.");
        return EFI_ABORTED;
    }
}

EFI_STATUS fread(EFI_FILE_HANDLE file, UINT64 size, VOID** buffer) {
    // Allocate buffer of kernel size
    EFI_STATUS status = BS->AllocatePool(EfiLoaderData, size, buffer);

    if(status != EFI_SUCCESS) {
        print_error(L"Efi program failed to allocate memory to read from a file.");
        return EFI_ABORTED;
    }

    // Read kernel into buffer
    status = file->Read(file, &size, *buffer);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"Efi program failed to read from a file.");
        return EFI_ABORTED;
    }
}

EFI_STATUS fclose(EFI_FILE_HANDLE file) {
    EFI_STATUS status = file->Close(file);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"Efi program failed to close a file.");
        return EFI_ABORTED;
    }
}

static EFI_STATUS init_graphics_fb(graphics_fb* fb) {
    EFI_STATUS status;

    // locate the gop protocol:
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    status = BS->LocateProtocol(&gop_guid, NULL, (void**) &gop);
    if(status != EFI_SUCCESS) {
        print_error(L"Efi program failed to locate graphics output protocol!");
        return status;
    }
    
    // This looks weird but it queries the current graphics output mode
    // to find the number of modes available. I don't think there is
    // a less confusing way of doing it!
    UINTN size_of_gop_mode_info;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* gop_mode_info;    
    status = gop->QueryMode(gop, gop->Mode == NULL ? 0 : gop->Mode->Mode, &size_of_gop_mode_info, &gop_mode_info);
    
    // To circumvent some buggy UEFI firmware, sets the video device
    // to the first mode available and clears display to black.
    if (status == EFI_NOT_STARTED) {
        status = gop->SetMode(gop, 0);
    }

    if(status != EFI_SUCCESS) {
        print_error(L"Efi program failed to query graphics output mode information.");
        return status;
    }

    // Currently unused: UINTN gop_native_mode = gop->Mode->Mode;
    UINTN gop_num_modes = gop->Mode->MaxMode;

    // query the available video modes:
    // we just try to find something as close to hd resolution (1280x720) as is available
    // and with pixel format TODO?
    INT32 select_mode = -1;
    int select_is_rgb_mode;
    UINT32 select_res_diff = 0xffffffff;

    for(UINTN i = 0; i < gop_num_modes; i++) {
        status = gop->QueryMode(gop, i, &size_of_gop_mode_info, &gop_mode_info);
        if(status != EFI_SUCCESS) {
            print_error(L"Efi program failed to query a graphics output mode!");
            return status;
        }

        int is_rgb_mode;
        if(gop_mode_info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) is_rgb_mode = 1;
        else if(gop_mode_info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) is_rgb_mode = 0;
        else continue;

        UINT32 res_diff = (UINT32)(abs(gop_mode_info->HorizontalResolution - DESIRED_FB_RESOLUTION_X) + abs(gop_mode_info->VerticalResolution - DESIRED_FB_RESOLUTION_Y));
        if(res_diff < select_res_diff) {
            select_mode = i;
            select_is_rgb_mode = is_rgb_mode;
            select_res_diff = res_diff;
        }
    }
    
    if(select_mode == -1) {
        print_error(L"Efi program could not find a suitable graphics output mode!");
        return EFI_ABORTED;
    }

    // set the video mode to the selected one:
    status = gop->SetMode(gop, (UINT32) select_mode);
    if(status != EFI_SUCCESS) {
        print_error(L"Efi program failed to set graphics output mode!");
        return status;
    }

    fb->base = (u32*) gop->Mode->FrameBufferBase;
    fb->size = gop->Mode->FrameBufferSize;
    fb->width = gop->Mode->Info->HorizontalResolution;
    fb->height = gop->Mode->Info->VerticalResolution;
    fb->stride = sizeof(int) * gop->Mode->Info->PixelsPerScanLine;

    return EFI_SUCCESS;
}

void print_u64(u64 v) {
    CHAR16 str[32];
    const CHAR16* hex = L"0123456789abcdef";
    int len = 0;
    u64 tmp = v;
    while (tmp != 0) {
        len++;
        tmp /= 16;
    }

    if (len == 0) {
        str[0] = hex[0];
        str[1] = 0;
    } else {
        tmp = v;
        for (int i = 0; i < len; i++) {
            str[len - 1 - i] = hex[tmp % 16];
            tmp /= 16;
        }
        str[len] = 0;
    }

    ST->ConOut->OutputString(ST->ConOut, str);
}

static void *get_acpi_table_ptr() {
    EFI_GUID acpi_guid = ACPI_20_TABLE_GUID;
    for (u32 i = 0; i < ST->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *efi_config = &ST->ConfigurationTable[i];
        if (memcmp(&efi_config->VendorGuid, &acpi_guid, sizeof(EFI_GUID)) == 0) {
            return efi_config->VendorTable;
        }
    }

    return null;
}

static EFI_STATUS get_memory_map(EFI_MEMORY_DESCRIPTOR *memory_map, UINTN *memory_map_size, 
    UINTN *memory_map_key, UINTN *desc_size, u32 *desc_version) {
    EFI_STATUS status = EFI_BUFFER_TOO_SMALL;
    *memory_map_size = sizeof(EFI_MEMORY_DESCRIPTOR) * 48;

    while (status == EFI_BUFFER_TOO_SMALL) {
        // Allocate some space for the efi memory map and efi memory descriptor. 
        *memory_map_size = sizeof(EFI_MEMORY_DESCRIPTOR) * 256;
        status = BS->AllocatePool(EfiLoaderData, (*memory_map_size) + 2 * (*desc_size), (void **) &memory_map);

        if (status != EFI_SUCCESS) {
            print_error(L"EFI program failed to allocate memory for memory map!");
            return status;
        }

        // Now store it into that allocated portion of memory.    
        status = BS->GetMemoryMap(memory_map_size, memory_map,
            memory_map_key, desc_size, desc_version);

        // We seem to have no way of knowing whether the buffer is large enough.
        // So we just try until we have an error or succeed.
        if (status == EFI_BUFFER_TOO_SMALL) {
            BS->FreePool(memory_map);   
            *memory_map_size += sizeof(EFI_MEMORY_DESCRIPTOR) * 16;         
        }
    }
    
    return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    ST = SystemTable;
    BS = ST->BootServices;

    // Configure the watchdog to reset the system after some seconds 
    // to restart the efi program if any bugs occur.
    BS->SetWatchdogTimer(4, 0, 0, NULL);

    // Reset the screen.
    ST->ConOut->Reset(ST->ConOut, FALSE);

    // Initialize the graphics output protocol and get a framebuffer.
    graphics_fb framebuffer;
    init_graphics_fb(&framebuffer);

    void *acpi_table_ptr = get_acpi_table_ptr();
    if (acpi_table_ptr == null) {
        print_error(L"EFI program failed to aquire acpi table pointer!");
        return EFI_ABORTED;
    }

    // Load kernel to address determined at link-time.
    kernel_main_ptr kernel_main = 0;
    status = load_kernel(ImageHandle, &kernel_main);
    if(status != EFI_SUCCESS) {
        print_error(L"EFI program failed to load the kernel!");
        return status;
    }

    EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
    UINTN memory_map_size = 0; 
    UINTN memory_map_key = 0;
    UINTN desc_size = 0; 
    u32 desc_version = 0;

    status = get_memory_map(memory_map, &memory_map_size, &memory_map_key, &desc_size, &desc_version);
    if (status != EFI_SUCCESS) {
        return status;
    }

    // Ensure we have the current memory map and disable the watchdog.
    status = BS->ExitBootServices(ImageHandle, memory_map_key);
    if (status != EFI_SUCCESS) {
        print_error(L"EFI program failed to exit boot services!");
        return status;
    }

    // Finally call into the kernel code which has been loaded to
    // the address designated by the linker.
    kernel_args args = {
        .framebuffer = framebuffer,
        .memory_map = memory_map,
        .memory_map_size = memory_map_size,
        .memory_map_desc_size = desc_size,
        .memory_map_desc_version = desc_version,
        .acpi_table_ptr = acpi_table_ptr
    };

    int kernel_status = kernel_main(args);

    if (kernel_status == 0) {
        return EFI_SUCCESS;
    } else {
        ST->ConOut->OutputString(ST->ConOut, L"ERROR: Kernel returned with error code ");
        print_u64((u64) kernel_status);
        ST->ConOut->OutputString(ST->ConOut, L"\n");
        return EFI_ABORTED;
    }
}
