CC=gcc
CFLAGS=-O3 -s -Wall

all:
	$(CC) $(CFLAGS) bf.c -o bf
