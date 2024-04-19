CC=gcc
CFLAGS=-Wall -g -Wvla -Werror

# Phony targets
.PHONY: all test filesystem clean

# Pattern rule for compiling C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Main all target
all: format filesystem test


# Build the test executable
test: test.o
	$(CC) -o test test.o -L. -lfilesystem

# Build the filesystem shared library
filesystem: filesystem.o
	gcc -Wall -fpic -c filesystem.c -o filesystem.o
	$(CC) -shared -o libfilesystem.so filesystem.o

# Clean rule to remove generated files
clean:
	rm -f *.o test libfilesystem.so format 2> /dev/null
