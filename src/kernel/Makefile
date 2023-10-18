# top-down passed variables
ADJ_KERNEL_FILE=../../$(KERNEL_FILE)

# local variables
CFLAGS+=-ffreestanding -mno-red-zone
LDFLAGS+=-no-pie -Wl,-e,main
C_FILES=$(wildcard *.c)
C_OBJECTS=$(patsubst %.c,%_c.o,$(C_FILES))

%_c.o: %.c
	clang $(CFLAGS) -c -o $@ $<

$(ADJ_KERNEL_FILE): $(C_OBJECTS)
	clang $(LDFLAGS) -o $@ $^

all: $(ADJ_KERNEL_FILE)

clean:
	rm -rf *.o