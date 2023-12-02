# top-down passed variables
ADJ_EFI_BOOT_FILE=../../$(EFI_BOOT_FILE)

# local variables
GNU_EFI_INC=/usr/include/efi
C_INCLUDES=-I$(GNU_EFI_INC) -I$(GNU_EFI_INC)/x86_64 -I$(GNU_EFI_INC)/protocol -I../common
CFLAGS+=-target x86_64-unknown-windows -fshort-wchar $(C_INCLUDES)
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