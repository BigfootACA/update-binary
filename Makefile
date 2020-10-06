CC?=aarch64-linux-gnu-gcc
CCFLAGS+=-pipe
CFLAGS+=-O2 -Wall -Wextra -Werror
LIBS+=-lzip -lz
LDFLAGS+=-s -static
all: update-binary
%.o: %.c
	$(CC) $(CCFLAGS) $(CFLAGS) -c $^ -o $@
update-binary.o: update-binary.c
update-binary: update-binary.o
	$(CC) $(CCFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
clean:
	rm -f *.o *.plist
	rm -f update-binary
.PHONY: all clean
