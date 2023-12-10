EFI_BOOT_FILE=bin/BOOTX64.EFI
export EFI_BOOT_FILE

KERNEL_FILE=bin/KERNEL
export KERNEL_FILE

IMAGE_FILE=bin/image.img
IMAGE_SIZE=64

GNU_EFI_INC=/usr/include/efi
C_INCLUDES=-I$(GNU_EFI_INC) -I$(GNU_EFI_INC)/x86_64 -I$(GNU_EFI_INC)/protocol -I../common
CFLAGS=-O0 -g -ffreestanding -mno-red-zone -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable $(C_INCLUDES)
export CFLAGS

ASM_FLAGS=-ffreestanding -Wall -Wextra
export ASM_FLAGS

LDFLAGS=-nostdlib
export LDFLAGS

SUBDIRS=src/elf_program src/kernel
.PHONY: $(SUBDIRS) all run clean

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

# NOTE: Essentially a lone partition with a fat32 filesystem
$(IMAGE_FILE): $(SUBDIRS)
	# dd if=/dev/zero of=$(IMAGE_FILE) bs=1024k count=$(IMAGE_SIZE) status=none
	# mformat -i $(IMAGE_FILE) -F ::
	# mmd -i $(IMAGE_FILE) ::/EFI
	# mmd -i $(IMAGE_FILE) ::/EFI/BOOT
	# mcopy -i $(IMAGE_FILE) $(EFI_BOOT_FILE) ::/EFI/BOOT
	# mcopy -i $(IMAGE_FILE) $(KERNEL_FILE) ::
	./tools/format_fat32/bin/format_fat32 bin/image2.img $(EFI_BOOT_FILE),/EFI/BOOT/BOOTX64.EFI $(KERNEL_FILE),/KERNEL

all: $(IMAGE_FILE)

QEMU_ARGS=-bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=$(IMAGE_FILE)
run:
ifeq ($(GDB),"1")
	qemu-system-x86_64 -s -S $(QEMU_ARGS)
else
	qemu-system-x86_64 $(QEMU_ARGS)
endif

clean: $(SUBDIRS)
	rm -rf bin/*
	touch bin/.gitkeep