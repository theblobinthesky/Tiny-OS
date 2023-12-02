# top-down passed variables
ADJ_KERNEL_FILE=../../$(KERNEL_FILE)

# local variables
C_INCLUDES=-I../common

# We target x86_64-unknown-windows to get the calling convention
# of the elf program and the kernel to match up. TODO: Hack.
CFLAGS+=-target x86_64-unknown-windows $(C_INCLUDES)
LDFLAGS+=-static -T kernel.lds
C_FILES=$(wildcard *.c)
C_OBJECTS=$(patsubst %.c,%_c.o,$(C_FILES))

%_c.o: %.c
	clang $(CFLAGS) -c -o $@ $<

$(ADJ_KERNEL_FILE): $(C_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

all: $(ADJ_KERNEL_FILE)

clean:
	rm -rf *.o