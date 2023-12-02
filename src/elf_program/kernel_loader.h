#pragma once
#include <efi.h>
#include <efilib.h>
#include "../kernel/kernel.h"

EFI_STATUS load_kernel(EFI_HANDLE image_handle, kernel_main_ptr *kernel_main);