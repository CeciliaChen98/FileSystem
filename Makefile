CC=gcc
CFLAGS=-Wall -g -Wvla -Werror

# Phony targets
.PHONY: all test filesystem job mysh clean

# Pattern rule for compiling C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Main all target
all: format filesystem job mysh test

# Build the test executable
test: test.o
	$(CC) $(CFLAGS) -o test test.o -L. -lfilesystem

# Build the filesystem shared library
filesystem: filesystem.o
	$(CC) $(CFLAGS) -fpic -c filesystem.c -o filesystem.o
	$(CC) -shared -o libfilesystem.so filesystem.o

# Build the job shared library
job: job.o
	$(CC) $(CFLAGS) -fpic -c job.c -o job.o
	$(CC) -shared -o libjob.so job.o

# Build the mysh executable
mysh: mysh.o 
	$(CC) $(CFLAGS) -o mysh mysh.o -L. -lfilesystem -ljob -lreadline -lpthread

# Clean rule to remove generated files
clean:
	rm -f *.o test mysh libfilesystem.so libjob.so format 2> /dev/null


