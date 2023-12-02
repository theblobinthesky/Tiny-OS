EFI_BOOT_FILE=bin/BOOTX64.EFI
export EFI_BOOT_FILE

KERNEL_FILE=bin/KERNEL
export KERNEL_FILE

IMAGE_FILE=bin/image.img
ISO_FILE=bin/cdrom.iso
IMAGE_SIZE=64

CFLAGS=-ffreestanding -mno-red-zone -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable
export CFLAGS

LDFLAGS=-nostdlib
export LDFLAGS

SUBDIRS=src/elf_program src/kernel
.PHONY: $(SUBDIRS) all run clean

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

# NOTE: Essentially a lone partition with a fat32 filesystem
$(ISO_FILE): $(SUBDIRS)
	dd if=/dev/zero of=$(IMAGE_FILE) bs=1024k count=$(IMAGE_SIZE) status=none
	mformat -i $(IMAGE_FILE) -F ::
	mmd -i $(IMAGE_FILE) ::/EFI
	mmd -i $(IMAGE_FILE) ::/EFI/BOOT
	mcopy -i $(IMAGE_FILE) $(EFI_BOOT_FILE) ::/EFI/BOOT
	mcopy -i $(IMAGE_FILE) $(KERNEL_FILE) ::
	
	rm -rf iso
	mkdir iso
	cp $(IMAGE_FILE) iso/fat.img
	xorriso -as mkisofs -R -f -e fat.img -no-emul-boot -o $(ISO_FILE) iso
	rm -rf iso

all: $(ISO_FILE)

run:
	$(MAKE) all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom $(ISO_FILE)

clean: $(SUBDIRS)
	rm -rf bin/*
	touch bin/.gitkeep