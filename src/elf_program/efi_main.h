#pragma once
#include <efi.h>

#define ASSERT_NO_EFI_ERROR() if(status != EFI_SUCCESS) return status;

void print_error(CHAR16* fmt);

EFI_STATUS fopen(EFI_FILE_HANDLE root_dir, CHAR16* file_name, UINT64 open_mode, UINT64 attributes, EFI_FILE_HANDLE* file);
EFI_STATUS fsize(EFI_FILE_HANDLE file, UINT64* file_size);
EFI_STATUS fread(EFI_FILE_HANDLE file, UINT64 size, VOID** buffer);
EFI_STATUS fclose(EFI_FILE_HANDLE file);