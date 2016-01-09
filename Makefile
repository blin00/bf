CC=gcc
# CC=x86_64-w64-mingw32-gcc
CFLAGS=-O3 -Wall

all:
	$(CC) $(CFLAGS) bf.c -o bf
