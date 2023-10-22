#include "../common/common.c"
#include <efi.h>
#include <efilib.h>
#include "kernel_loader.h"
#include "efiConsoleControl.h"

void print(CHAR16* fmt, ...) {
    ST->ConOut->OutputString(ST->ConOut, fmt);
}

void print_error(CHAR16* fmt) {
    print(L"ERROR: ");
    print(fmt);
}

static void wait_forever() {
    while(TRUE) {
        __asm__ volatile("nop");
    }
}

EFI_STATUS fopen(EFI_FILE_HANDLE root_dir, CHAR16* file_name, UINT64 open_mode, UINT64 attributes, EFI_FILE_HANDLE* file) {
    EFI_STATUS status = root_dir->Open(root_dir, file, file_name, open_mode, attributes);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"fopen() failed.\n");
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
        print_error(L"fsize() failed.\n");
        return EFI_ABORTED;
    }
}

EFI_STATUS fread(EFI_FILE_HANDLE file, UINT64 size, VOID** buffer) {
    // Allocate buffer of kernel size
    EFI_STATUS status = BS->AllocatePool(EfiLoaderData, size, buffer);

    if(status != EFI_SUCCESS) {
        print_error(L"BS->AllocatePool() failed.\n");
        return EFI_ABORTED;
    }

    // Read kernel into buffer
    status = file->Read(file, &size, *buffer);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"fread() failed.\n");
        return EFI_ABORTED;
    }
}

EFI_STATUS fclose(EFI_FILE_HANDLE file) {
    EFI_STATUS status = file->Close(file);

    if(status == EFI_SUCCESS) {
        return EFI_SUCCESS;
    } else {
        print_error(L"fclose() failed.\n");
        return EFI_ABORTED;
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    ST = SystemTable;
    BS = ST->BootServices;

    BS->SetWatchdogTimer(4, 0, 0, NULL); // reset the system after 4 seconds if BS->ExitBootServices has not been called.
    ST->ConOut->Reset(ST->ConOut, FALSE); // clear screen and reset cursor to (0,0)

    status = load_kernel(ImageHandle);
    if(status != EFI_SUCCESS) goto efi_abort;

    print(L"Still running...\n");
    wait_forever();

efi_abort:
    // If we reach this the efi program has failed to call into the kernel
    print(L"EFI Program failed to call into the kernel.\n");
    return EFI_ABORTED;
}
