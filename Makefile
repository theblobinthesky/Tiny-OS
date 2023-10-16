MAKEFILE=Makefile
CFLAGS=-target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -Iinclude/gnu_efi/x86_64 -Iinclude/gnu_efi/protocol -Iinclude/gnu_efi
LDFLAGS=-target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main -Wl,-subsystem:efi_application -fuse-ld=lld-link
EFI_FILE=bin/BOOTX64.EFI

C_FILES = $(wildcard src/*.c)
C_OBJECTS = $(patsubst %.c,%_c.o,$(C_FILES))

%_c.o: %.c $(MAKEFILE)
	clang $(CFLAGS) -c -o $@ $<

$(EFI_FILE): $(C_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

all: $(EFI_FILE)

clean:
	rm src/*.o