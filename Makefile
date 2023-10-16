CFLAGS=-target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -Iinclude/gnu_efi/x86_64 -Iinclude/gnu_efi/protocol -Iinclude/gnu_efi
LDFLAGS=-target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main -Wl,-subsystem:efi_application -fuse-ld=lld-link
EFI_FILE=bin/BOOTX64.EFI
IMAGE_FILE=bin/image.img
ISO_FILE=bin/image.iso
IMAGE_SIZE=64

C_FILES = $(wildcard src/*.c)
C_OBJECTS = $(patsubst %.c,%_c.o,$(C_FILES))

%_c.o: %.c
	clang $(CFLAGS) -c -o $@ $<

$(EFI_FILE): $(C_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

# NOTE: A MBR partition table with a fat32 filesystem
all: $(EFI_FILE)
	dd if=/dev/zero of=$(IMAGE_FILE) bs=1024k count=$(IMAGE_SIZE) status=none
	mformat -i $(IMAGE_FILE) -F ::
	mmd -i $(IMAGE_FILE) ::/EFI
	mmd -i $(IMAGE_FILE) ::/EFI/BOOT
	mcopy -i $(IMAGE_FILE) $(EFI_FILE) ::/EFI/BOOT
	
run: all
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom $(ISO_FILE)

clean:
	rm -rf src/*.o
	rm -rf bin/*