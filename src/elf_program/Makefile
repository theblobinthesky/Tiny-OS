# top-down passed variables
ADJ_EFI_BOOT_FILE=../../$(EFI_BOOT_FILE)

# local variables
CFLAGS+=-target x86_64-unknown-windows -fshort-wchar
LDFLAGS+=-target x86_64-unknown-windows -Wl,-entry:efi_main -Wl,-subsystem:efi_application -fuse-ld=lld-link
C_FILES=$(wildcard *.c)
C_OBJECTS=$(patsubst %.c,%_c.o,$(C_FILES))

%_c.o: %.c
	clang $(CFLAGS) -c -o $@ $<

$(ADJ_EFI_BOOT_FILE): $(C_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

all: $(ADJ_EFI_BOOT_FILE)

clean:
	rm -rf *.o