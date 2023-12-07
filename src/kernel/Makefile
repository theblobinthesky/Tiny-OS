# top-down passed variables
ADJ_KERNEL_FILE=../../$(KERNEL_FILE)

# local variables
C_INCLUDES=-I../common

# We target x86_64-unknown-windows to get the calling convention
# of the elf program and the kernel to match up. TODO: Hack.
CFLAGS+=-target x86_64-unknown-windows $(C_INCLUDES)
ASM_FLAGS+=-target x86_64-unknown-windows
LDFLAGS+=-static -T kernel.lds
C_FILES=$(wildcard *.c)
C_OBJECTS=$(patsubst %.c,%_c.o,$(C_FILES))
ASM_FILES=$(wildcard *.S)
ASM_OBJECTS=$(patsubst %.S,%_S.o,$(ASM_FILES))

%_c.o: %.c
	clang $(CFLAGS) -c -o $@ $<

%_S.o: %.S
	clang $(ASM_FLAGS) -c -o $@ $<

$(ADJ_KERNEL_FILE): $(C_OBJECTS) $(ASM_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

all: $(ADJ_KERNEL_FILE)

clean:
	rm -rf *.o