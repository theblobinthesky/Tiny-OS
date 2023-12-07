BIN_FILE=bin/format_fat32

CFLAGS=
LDFLAGS=

C_FILES=$(wildcard src/*.cpp)
C_OBJECTS=$(patsubst %.cpp,%.o,$(C_FILES))

%_c.o: %.c
	g++ $(CFLAGS) -c -o $@ $<

$(BIN_FILE): $(C_OBJECTS)
	g++ $(LDFLAGS) -o $@ $^

all: $(BIN_FILE)

clean:
	rm -r $(BIN_FILE)
	rm -rf src/*.o